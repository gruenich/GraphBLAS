//------------------------------------------------------------------------------
// GB_select_sparse:  select entries from a matrix (C is sparse/hypersparse)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64-bit.

#define GB_DEBUG

#include "select/GB_select.h"
#ifndef GBCOMPACT
#include "FactoryKernels/GB_sel__include.h"
#endif
#include "scalar/GB_Scalar_wrap.h"
#include "jitifyer/GB_stringify.h"
#include "slice/factory/GB_ek_slice_merge.h"

#define GB_FREE_WORKSPACE                   \
{                                           \
    GB_FREE_WORK (&Zp, Zp_size) ;           \
    GB_WERK_POP (Work, int64_t) ;           \
    GB_WERK_POP (A_ek_slicing, int64_t) ;   \
}

#define GB_FREE_ALL                         \
{                                           \
    GB_phybix_free (C) ;                    \
    GB_FREE_WORKSPACE ;                     \
}

GrB_Info GB_select_sparse
(
    GrB_Matrix C,                   // output matrix; empty header on input
    const bool C_iso,               // if true, construct C as iso
    const GrB_IndexUnaryOp op,
    const bool flipij,              // if true, flip i and j for the op
    const GrB_Matrix A,             // input matrix
    const int64_t ithunk,           // input scalar, cast to int64_t
    const GB_void *restrict athunk, // same input scalar, but cast to A->type
    const GB_void *restrict ythunk, // same input scalar, but cast to op->ytype
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    // C is always an empty header on input.  A is never bitmap.  It is
    // sparse/hypersparse, with one exception: for the DIAG operator, A may be
    // sparse, hypersparse, or full.

    ASSERT (C != NULL && (C->static_header || GBNSTATIC)) ;
    ASSERT_MATRIX_OK (A, "A input for GB_select_sparse", GB0) ;
    ASSERT_INDEXUNARYOP_OK (op, "op for GB_select_sparse", GB0) ;
    ASSERT (!GB_IS_BITMAP (A)) ;
    ASSERT (GB_IS_SPARSE (A) || GB_IS_HYPERSPARSE (A) || GB_IS_FULL (A)) ;
    ASSERT (GB_IMPLIES (op->opcode != GB_DIAG_idxunop_code,
        GB_IS_SPARSE (A) || GB_IS_HYPERSPARSE (A))) ;

    //--------------------------------------------------------------------------
    // declare workspace
    //--------------------------------------------------------------------------

    GrB_Info info ;
    uint64_t *restrict Zp = NULL ; size_t Zp_size = 0 ;     // FIXME
    GB_WERK_DECLARE (Work, int64_t) ;
    int64_t *restrict Wfirst = NULL ;
    int64_t *restrict Wlast = NULL ;
    int64_t *restrict Cp_kfirst = NULL ;
    GB_WERK_DECLARE (A_ek_slicing, int64_t) ;

    GB_Opcode opcode = op->opcode ;
    const bool A_iso = A->iso ;
    const GB_Type_code acode = A->type->code ;

    //--------------------------------------------------------------------------
    // determine the max number of threads to use
    //--------------------------------------------------------------------------

    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;

    //--------------------------------------------------------------------------
    // get A: sparse, hypersparse, or full
    //--------------------------------------------------------------------------

    int64_t anvec = A->nvec ;
    bool A_is_hyper = GB_IS_HYPERSPARSE (A) ;

    //--------------------------------------------------------------------------
    // create the C matrix
    //--------------------------------------------------------------------------

    int csparsity = (A_is_hyper) ? GxB_HYPERSPARSE : GxB_SPARSE ;

    GB_OK (GB_new (&C, // sparse or hyper (from A), existing header
        A->type, A->vlen, A->vdim, GB_ph_calloc, A->is_csc,
        csparsity, A->hyper_switch, A->plen,
        /* FIXME: */ false, false)) ;

    if (A_is_hyper)
    { 
        // C->h is a deep copy of A->h
        GB_memcpy (C->h, A->h, A->nvec * sizeof (uint64_t), nthreads_max) ;
    }

    C->nvec = A->nvec ;
    C->nvals = 0 ;
    C->magic = GB_MAGIC ;

    // C->Y is not yet constructed
    ASSERT (C->Y == NULL) ;
    ASSERT_MATRIX_OK (C, "C initialized as empty for GB_selector", GB0) ;
    ASSERT (C->i == NULL) ;
    ASSERT (C->x == NULL) ;

    C->iso = C_iso ;

    //--------------------------------------------------------------------------
    // slice the entries for each task
    //--------------------------------------------------------------------------

    int A_ntasks, A_nthreads ;
    int64_t anz_held = GB_nnz_held (A) ;
    double work = 8*anvec + ((opcode == GB_DIAG_idxunop_code) ? 0 : anz_held) ;
    GB_SLICE_MATRIX_WORK (A, 8, work, anz_held) ;

    //--------------------------------------------------------------------------
    // allocate workspace for each task
    //--------------------------------------------------------------------------

    GB_WERK_PUSH (Work, 3*A_ntasks, int64_t) ;
    if (Work == NULL)
    { 
        // out of memory
        GB_FREE_ALL ;
        return (GrB_OUT_OF_MEMORY) ;
    }
    Wfirst    = Work ;
    Wlast     = Work + A_ntasks ;
    Cp_kfirst = Work + A_ntasks * 2 ;

    //--------------------------------------------------------------------------
    // allocate workspace for phase1
    //--------------------------------------------------------------------------

    // phase1 counts the number of live entries in each vector of A.  The
    // result is computed in Cp, where Cp [k] is the number of live entries in
    // the kth vector of A.  Zp [k] is the location of the A(i,k) entry, for
    // positional operators.

    bool op_is_positional = GB_IS_INDEXUNARYOP_CODE_POSITIONAL (opcode) ;
    if (op_is_positional)
    {
        // allocate Zp
        Zp = GB_MALLOC_WORK (C->plen + 1, uint64_t, &Zp_size) ; // FIXME
        if (Zp == NULL)
        { 
            // out of memory
            GB_FREE_ALL ;
            return (GrB_OUT_OF_MEMORY) ;
        }
    }

    //==========================================================================
    // phase1: count the live entries in each column
    //==========================================================================

    info = GrB_NO_VALUE ;
    if (op_is_positional || opcode == GB_NONZOMBIE_idxunop_code)
    { 

        //----------------------------------------------------------------------
        // positional ops or nonzombie phase1 do not depend on the values
        //----------------------------------------------------------------------

        // no JIT worker needed for these operators
        info = GB_select_positional_phase1 (C, Zp, Wfirst, Wlast, A, ithunk,
            op, A_ek_slicing, A_ntasks, A_nthreads) ;

    }
    else
    {

        //----------------------------------------------------------------------
        // entry selectors depend on the values in phase1
        //----------------------------------------------------------------------

        ASSERT (!A_iso || opcode == GB_USER_idxunop_code) ;
        ASSERT ((opcode >= GB_VALUENE_idxunop_code
             && opcode <= GB_VALUELE_idxunop_code)
             || (opcode == GB_USER_idxunop_code)) ;

        #ifndef GBCOMPACT
        GB_IF_FACTORY_KERNELS_ENABLED
        { 

            //------------------------------------------------------------------
            // via the factory kernel (includes user-defined ops)
            //------------------------------------------------------------------

            // define the worker for the switch factory
            #define GB_sel1(opname,aname) GB (_sel_phase1_ ## opname ## aname)
            #define GB_SEL_WORKER(opname,aname)                             \
            {                                                               \
                info = GB_sel1 (opname, aname) (C, Wfirst, Wlast, A,        \
                    ythunk, A_ek_slicing, A_ntasks, A_nthreads) ;           \
            }                                                               \
            break ;

            // launch the switch factory
            #include "select/factory/GB_select_entry_factory.c"
            #undef  GB_SEL_WORKER
        }
        #endif

        //----------------------------------------------------------------------
        // via the JIT or PreJIT kernel
        //----------------------------------------------------------------------

        if (info == GrB_NO_VALUE)
        { 
            info = GB_select_phase1_jit (C, Wfirst, Wlast, A, ythunk, op,
                flipij, A_ek_slicing, A_ntasks, A_nthreads) ;
        }

        //----------------------------------------------------------------------
        // via the generic kernel
        //----------------------------------------------------------------------

        if (info == GrB_NO_VALUE)
        { 
            // generic entry selector, phase1
            GBURBLE ("(generic select) ") ;
            info = GB_select_generic_phase1 (C, Wfirst, Wlast, A, flipij,
                ythunk, op, A_ek_slicing, A_ntasks, A_nthreads) ;
        }
    }

    GB_OK (info) ;  // check for out-of-memory or other failures in phase1

    //==========================================================================
    // phase1b: cumulative sum and allocate C
    //==========================================================================

    //--------------------------------------------------------------------------
    // cumulative sum of Cp and compute Cp_kfirst
    //--------------------------------------------------------------------------

    uint64_t *restrict Cp = C->p ;      // FIXME
    int64_t C_nvec_nonempty ;
    if (!op_is_positional)
    { 
        // GB_select_positional_phase1 finalizes Cp in the
        // select/factory/GB_select_positional_phase1_template.c.  This phase
        // is only needed for entry-style selectors, done by
        // select/template/GB_select_entry_phase1_template.c:
        GB_ek_slice_merge1 (Cp, /* FIXME: */ false,
            Wfirst, Wlast, A_ek_slicing, A_ntasks) ;
    }

    // All kernels need to this phase to compute cumsum(Cp) and Cp_kfirst:
    GB_ek_slice_merge2 (&C_nvec_nonempty, Cp_kfirst,
        Cp, /* FIXME: */ false,
        anvec, Wfirst, Wlast, A_ek_slicing, A_ntasks, A_nthreads, Werk) ;

    //--------------------------------------------------------------------------
    // allocate new space for the compacted C->i and C->x
    //--------------------------------------------------------------------------

    C->nvec_nonempty = C_nvec_nonempty ;
    uint64_t cnz = Cp [anvec] ; // FIXME
    GB_OK (GB_bix_alloc (C, cnz, csparsity, false, true, C_iso)) ;
    C->jumbled = A->jumbled ;
    C->nvals = cnz ;

    //--------------------------------------------------------------------------
    // set the iso value of C
    //--------------------------------------------------------------------------

    if (C_iso)
    { 
        // The pattern of C is computed by the worker below.
        GB_select_iso (C->x, opcode, athunk, A->x, A->type->size) ;
    }

    //==========================================================================
    // phase2: select the entries
    //==========================================================================

    info = GrB_NO_VALUE ;
    if (op_is_positional || (opcode == GB_NONZOMBIE_idxunop_code && A_iso))
    { 

        //----------------------------------------------------------------------
        // positional ops do not depend on the values
        //----------------------------------------------------------------------

        // no JIT worker needed for these operators
        info = GB_select_positional_phase2 (C, Zp, Cp_kfirst, A, flipij,
            ithunk, op, A_ek_slicing, A_ntasks, A_nthreads) ;

    }
    else
    {

        //----------------------------------------------------------------------
        // entry selectors depend on the values in phase2
        //----------------------------------------------------------------------

        ASSERT (!A_iso || opcode == GB_USER_idxunop_code) ;
        ASSERT ((opcode >= GB_VALUENE_idxunop_code &&
                 opcode <= GB_VALUELE_idxunop_code)
             || (opcode == GB_NONZOMBIE_idxunop_code && !A_iso)
             || (opcode == GB_USER_idxunop_code)) ;

        #ifndef GBCOMPACT
        GB_IF_FACTORY_KERNELS_ENABLED
        { 

            //------------------------------------------------------------------
            // via the factory kernel
            //------------------------------------------------------------------

            // define the worker for the switch factory
            #define GB_SELECT_PHASE2
            #define GB_sel2(opname,aname) GB (_sel_phase2_ ## opname ## aname)
            #define GB_SEL_WORKER(opname,aname)                             \
            {                                                               \
                info = GB_sel2 (opname, aname) (C, Cp_kfirst, A, ythunk,    \
                    A_ek_slicing, A_ntasks, A_nthreads) ;                   \
            }                                                               \
            break ;

            // launch the switch factory
            #include "select/factory/GB_select_entry_factory.c"
        }
        #endif

        //----------------------------------------------------------------------
        // via the JIT or PreJIT kernel
        //----------------------------------------------------------------------

        if (info == GrB_NO_VALUE)
        { 
            info = GB_select_phase2_jit (C, Cp_kfirst, A, flipij, ythunk, op,
                A_ek_slicing, A_ntasks, A_nthreads) ;
        }

        //----------------------------------------------------------------------
        // via the generic kernel
        //----------------------------------------------------------------------

        if (info == GrB_NO_VALUE)
        { 
            // generic entry selector, phase2
            info = GB_select_generic_phase2 (C, Cp_kfirst, A, flipij, ythunk,
                op, A_ek_slicing, A_ntasks, A_nthreads) ;
        }
    }

    GB_OK (info) ;  // phase2 cannot fail, but check just in case

    //==========================================================================
    // finalize the result, free workspace, and return result
    //==========================================================================

    GB_FREE_WORKSPACE ;
    ASSERT_MATRIX_OK (C, "C before hyper_prune for GB_selector", GB0) ;
    GB_OK (GB_hyper_prune (C, Werk)) ;
    ASSERT_MATRIX_OK (C, "C output for GB_selector", GB0) ;
    return (GrB_SUCCESS) ;
}

