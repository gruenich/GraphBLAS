//------------------------------------------------------------------------------
// GB_subref_phase3: C=A(I,J) where C and A are sparse/hypersparse
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit

// This function either frees Cp and Ch, or transplants then into C, as C->p
// and C->h.  Either way, the caller must not free them.

#include "extract/GB_subref.h"
#include "sort/GB_sort.h"
#include "jitifyer/GB_stringify.h"

GrB_Info GB_subref_phase3   // C=A(I,J)
(
    GrB_Matrix C,               // output matrix, static header
    // from phase2:
    uint64_t **Cp_handle,       // vector pointers for C FIXME
    size_t Cp_size,
    const int64_t Cnvec_nonempty,       // # of non-empty vectors in C
    // from phase1:
    const GB_task_struct *restrict TaskList,    // array of structs
    const int ntasks,                           // # of tasks
    const int nthreads,                         // # of threads to use
    const bool post_sort,               // true if post-sort needed
    const int64_t *Mark,                // for I inverse buckets, size A->vlen
    const int64_t *Inext,               // for I inverse buckets, size nI
    const bool I_has_duplicates,        // true if I has duplicates
    // from phase0:
    int64_t **Ch_handle,
    size_t Ch_size,
    const uint64_t *restrict Ap_start,
    const uint64_t *restrict Ap_end,
    const int64_t Cnvec,
    const bool need_qsort,
    const int Ikind,
    const int64_t nI,
    const int64_t Icolon [3],
    const int64_t nJ,
    // from GB_subref:
    const bool C_iso,           // if true, C is iso
    const GB_void *cscalar,     // iso value of C
    // original input:
    const bool C_is_csc,        // format of output matrix C
    const GrB_Matrix A,
    const GrB_Index *I,
    const bool symbolic,
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (C != NULL && (C->static_header || GBNSTATIC)) ;
    ASSERT (Cp_handle != NULL) ;
    ASSERT (Ch_handle != NULL) ;
    const uint64_t *restrict Cp = (*Cp_handle) ;    // FIXME
    const int64_t *restrict Ch = (*Ch_handle) ;
    ASSERT (Cp != NULL) ;
    ASSERT_MATRIX_OK (A, "A for subref phase3", GB0) ;
    ASSERT (!GB_IS_BITMAP (A)) ;
    ASSERT (!GB_IS_FULL (A)) ;
    ASSERT (GB_IS_SPARSE (A) || GB_IS_HYPERSPARSE (A)) ;

    //--------------------------------------------------------------------------
    // allocate the output matrix C
    //--------------------------------------------------------------------------

    int64_t cnz = Cp [Cnvec] ;
    bool C_is_hyper = (Ch != NULL) ;
    GrB_Type ctype = (symbolic) ? GrB_INT64 : A->type ;

    // allocate the result C (but do not allocate C->p or C->h)
    int sparsity = C_is_hyper ? GxB_HYPERSPARSE : GxB_SPARSE ;
    GrB_Info info = GB_new_bix (&C, // sparse or hyper, existing header
        ctype, nI, nJ, GB_ph_null, C_is_csc,
        sparsity, true, A->hyper_switch, Cnvec, cnz, true, C_iso,
        /* FIXME: */ false, false) ;
    if (info != GrB_SUCCESS)
    { 
        // out of memory
        GB_FREE (Cp_handle, Cp_size) ;
        GB_FREE (Ch_handle, Ch_size) ;
        return (info) ;
    }

    // add Cp as the vector pointers for C, from GB_subref_phase2
    C->p = (int64_t *) Cp ; C->p_size = Cp_size ;
    (*Cp_handle) = NULL ;

    // add Ch as the hypersparse list for C, from GB_subref_phase0
    if (C_is_hyper)
    { 
        // transplant Ch into C
        C->h = (int64_t *) Ch ; C->h_size = Ch_size ;
        (*Ch_handle) = NULL ;
        C->nvec = Cnvec ;
    }

    // now Cp and Ch have been transplanted into C, so they must not be freed.
    ASSERT ((*Cp_handle) == NULL) ;
    ASSERT ((*Ch_handle) == NULL) ;
    C->nvec_nonempty = Cnvec_nonempty ;
    C->nvals = cnz ;
    C->magic = GB_MAGIC ;

    //--------------------------------------------------------------------------
    // phase3: C = A(I,J)
    //--------------------------------------------------------------------------

    #define GB_PHASE_2_OF_2
    int64_t *restrict Ci = C->i ;   // FIXME
    int64_t *restrict Cx = (int64_t *) C->x ;
    #define GB_I_KIND Ikind
    #define GB_NEED_QSORT need_qsort
    #define GB_I_HAS_DUPLICATES I_has_duplicates

    if (symbolic)
    { 

        //----------------------------------------------------------------------
        // symbolic subref
        //----------------------------------------------------------------------

        ASSERT (!C_iso) ;

        // symbolic subref must handle zombies
        const bool may_see_zombies = (A->nzombies > 0) ;

        // symbolic copy: Cx is int64_t; the values of A ignored
        #define GB_COPY_RANGE(pC,pA,len)            \
            for (int64_t k = 0 ; k < (len) ; k++)   \
            {                                       \
                Cx [(pC) + k] = (pA) + k ;          \
            }
        #define GB_COPY_ENTRY(pC,pA) Cx [pC] = (pA) ;
        #define GB_QSORT_1B(Ci,Cx,pC,clen)                  \
            GB_qsort_1b_64_size8 ((uint64_t *) (Ci + pC),   \
                (uint64_t *) (Cx + pC), clen) ;
        #define GB_SYMBOLIC
        #include "extract/template/GB_subref_template.c"

    }
    else if (C_iso)
    { 

        //----------------------------------------------------------------------
        // iso numeric subref
        //----------------------------------------------------------------------

        // C is iso; no numeric values to extract; just set the iso value
        memcpy (Cx, cscalar, A->type->size) ;
        #define GB_COPY_RANGE(pC,pA,len) ;
        #define GB_COPY_ENTRY(pC,pA) ;
        #define GB_ISO_SUBREF
        #define GB_QSORT_1B(Ci,Cx,pC,clen) ;
        #include "extract/template/GB_subref_template.c"

    }
    else
    { 

        //----------------------------------------------------------------------
        // non-iso numeric subref
        //----------------------------------------------------------------------

        // using the JIT kernel
        info = GB_subref_sparse_jit (C, TaskList, ntasks, nthreads, post_sort,
            Mark, Inext, I_has_duplicates, Ap_start, Ap_end, need_qsort,
            Ikind, nI, Icolon, A, I) ;

        if (info == GrB_NO_VALUE)
        { 
            // using the generic kernel
            GBURBLE ("(generic subref) ") ;
            ASSERT (C->type = A->type) ;
            const int64_t csize = C->type->size ;
            const GB_void *restrict Ax = (GB_void *) A->x ;
                  GB_void *restrict Cx = (GB_void *) C->x ;

            // C and A have the same type
            #define GB_COPY_RANGE(pC,pA,len)                                \
                memcpy (Cx + (pC)*csize, Ax + (pA)*csize, (len) * csize) ;
            #define GB_COPY_ENTRY(pC,pA)                                    \
                memcpy (Cx + (pC)*csize, Ax + (pA)*csize, csize) ;
            #define GB_QSORT_1B(Ci,Cx,pC,clen)                  \
                GB_qsort_1b_64_generic ((uint64_t *) (Ci+(pC)), \
                    (GB_void *) (Cx+(pC)*csize), csize, clen) ;
            #include "extract/template/GB_subref_template.c"
            info = GrB_SUCCESS ;
        }
    }

    //--------------------------------------------------------------------------
    // remove empty vectors from C, if hypersparse
    //--------------------------------------------------------------------------

    if (info == GrB_SUCCESS)
    { 
        info = GB_hyper_prune (C, Werk) ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    if (info != GrB_SUCCESS)
    { 
        // out of memory or JIT kernel failed
        GB_phybix_free (C) ;
        return (info) ;
    }

    // caller must not free Cp or Ch
    ASSERT_MATRIX_OK (C, "C output for subref phase3", GB0) ;
    return (GrB_SUCCESS) ;
}

