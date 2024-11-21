//------------------------------------------------------------------------------
// GB_selector:  select entries from a matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// GB_selector does the work for GB_select.  It also deletes zombies for
// GB_wait using the GxB_NONZOMBIE operator, deletes entries outside a smaller
// matrix for GxB_*resize using GrB_ROWLE, and extracts the diagonal entries
// for GB_Vector_diag.

// For GB_resize (using GrB_ROWLE) and GB_wait (using GxB_NONZOMBIE), C may be
// NULL.  In this case, A is always sparse or hypersparse.  If C is NULL on
// input, A is modified in-place.  Otherwise, C is an uninitialized static
// header.

// TODO: GB_selector does not exploit the mask.

#include "select/GB_select.h"

#define GB_FREE_ALL ;

static int run = 0;

GrB_Info GB_selector
(
    GrB_Matrix C,               // output matrix, NULL or existing header
    const GrB_IndexUnaryOp op,
    const bool flipij,          // if true, flip i and j for user operator
    GrB_Matrix A,               // input matrix
    const GrB_Scalar Thunk,
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_INDEXUNARYOP_OK (op, "idxunop for GB_selector", GB0) ;
    ASSERT_SCALAR_OK (Thunk, "Thunk for GB_selector", GB0) ;
    ASSERT_MATRIX_OK (A, "A input for GB_selector", GB_ZOMBIE (GB0)) ;
    // positional op (tril, triu, diag, offdiag, resize, rowindex, ...):
    // can't be jumbled.  nonzombie, entry-valued op, user op: jumbled OK
    GB_Opcode opcode = op->opcode ;
    ASSERT (GB_IMPLIES (GB_IS_INDEXUNARYOP_CODE_POSITIONAL (opcode),
        !GB_JUMBLED (A))) ;
    ASSERT (C == NULL || (C != NULL && (C->static_header || GBNSTATIC))) ;

    bool in_place_A = (C == NULL) ; // GrB_wait and GB_resize only
    const bool A_iso = A->iso ;

    //--------------------------------------------------------------------------
    // get Thunk
    //--------------------------------------------------------------------------

    // get the type of the thunk input of the operator
    ASSERT (GB_nnz ((GrB_Matrix) Thunk) > 0) ;
    const GB_Type_code tcode = Thunk->type->code ;

    // ythunk = (op->ytype) Thunk
    size_t ysize = op->ytype->size ;
    GB_void ythunk [GB_VLA(ysize)] ;
    memset (ythunk, 0, ysize) ;
    GB_cast_scalar (ythunk, op->ytype->code, Thunk->x, tcode, ysize) ;

    // ithunk = (int64) Thunk, if compatible
    int64_t ithunk = 0 ;
    if (GB_Type_compatible (GrB_INT64, Thunk->type))
    {
        GB_cast_scalar (&ithunk, GB_INT64_code, Thunk->x, tcode,
            sizeof (int64_t)) ;
    }

    // athunk = (A->type) Thunk, for VALUEEQ operator only
    const size_t asize = A->type->size ;
    GB_void athunk [GB_VLA(asize)] ;
    memset (athunk, 0, asize) ;
    if (opcode == GB_VALUEEQ_idxunop_code)
    {
        ASSERT (GB_Type_compatible (A->type, Thunk->type)) ;
        GB_cast_scalar (athunk, A->type->code, Thunk->x, tcode, asize) ;
    }

    //--------------------------------------------------------------------------
    // determine if C is iso for a non-iso A
    //--------------------------------------------------------------------------

    bool C_iso = A_iso ||                       // C iso value is Ax [0]
        (opcode == GB_VALUEEQ_idxunop_code) ;   // C iso value is thunk
    if (C_iso)
    { 
        GB_BURBLE_MATRIX (A, "(iso select) ") ;
    }

    //--------------------------------------------------------------------------
    // handle iso case for built-in ops that depend only on the value
    //--------------------------------------------------------------------------

    if (A_iso && opcode >= GB_VALUENE_idxunop_code
              && opcode <= GB_VALUELE_idxunop_code)
    { 
        return (GB_select_value_iso (C, op, A, ithunk, athunk, ythunk, Werk)) ;
    }

    //--------------------------------------------------------------------------
    // bitmap/as-if-full case
    //--------------------------------------------------------------------------

    bool use_select_bitmap ;
    if (opcode == GB_NONZOMBIE_idxunop_code || in_place_A)
    { 
        // GB_select_bitmap does not support the nonzombie opcode, nor does
        // it support operating on A in place.  For the NONZOMBIE operator, A
        // will never be bitmap.
        use_select_bitmap = false ;
    }
    else if (opcode == GB_DIAG_idxunop_code)
    { 
        // GB_select_bitmap supports the DIAG operator, but it is currently
        // not efficient (GB_select_bitmap should return a sparse diagonal
        // matrix, not bitmap).  So use the sparse case if A is not bitmap,
        // since the sparse case below does not support the bitmap case.
        use_select_bitmap = GB_IS_BITMAP (A) ;
    }
    else
    { 
        // For bitmap, full, or as-if-full matrices (sparse/hypersparse with
        // all entries present, not jumbled, no zombies, and no pending
        // tuples), use the bitmap selector for all other operators (TRIL,
        // TRIU, OFFDIAG, NONZERO, EQ*, GT*, GE*, LT*, LE*, and user-defined
        // operators).
        use_select_bitmap = GB_IS_BITMAP (A) || GB_IS_FULL (A) ;
    }

    if (use_select_bitmap)
    { 
        GB_BURBLE_MATRIX (A, "(bitmap select) ") ;
        ASSERT (C != NULL && (C->static_header || GBNSTATIC)) ;
        return (GB_select_bitmap (C, C_iso, op,                  
            flipij, A, ithunk, athunk, ythunk, Werk)) ;
    }

    //--------------------------------------------------------------------------
    // column selector
    //--------------------------------------------------------------------------

    if (opcode == GB_COLINDEX_idxunop_code ||
        opcode == GB_COLLE_idxunop_code ||
        opcode == GB_COLGT_idxunop_code)
    { 
        return (GB_select_column (C, C_iso, op, A, ithunk, Werk)) ;
    }

    //--------------------------------------------------------------------------
    // sparse/hypersparse general case
    //--------------------------------------------------------------------------

    info = GrB_NO_VALUE ;

    // FIXME: pass in a T matrix, below, not C
    struct GB_Matrix_opaque T_header ;
    GrB_Matrix T ;
    GB_CLEAR_STATIC_HEADER (T, &T_header) ;

    bool compare = false ;

    #if defined ( GRAPHBLAS_HAS_CUDA )
    if (!in_place_A /* FIXME: Workaround for CUDA kernel not
        handling in-place condition. Fix by building result
        in T matrix for both CUDA and CPU and handle in-place case
        separately at end */
        && (GB_IS_SPARSE (A) || GB_IS_HYPERSPARSE (A)) /* It is possible for
        non-sparse matrices to use the sparse kernel; see check for 
        use_select_bitmap above. The CUDA select_sparse kernel will not work
        in this case, so make this go to the CPU. */
        && GB_cuda_select_branch (A, op))
    {
        compare = true ;
        info = GB_cuda_select_sparse (T, C_iso, op, flipij, A, ythunk) ;
    }
    #endif
    // Failing on TriangleCount, Cached_NDiag
    if (/* info == GrB_NO_VALUE */ true)
    {
        run++;
        bool fallout = false;
        #define SAME(a, b, fmt, args...)                                            \
        {                                                                           \
            if (fallout) {                                                          \
                printf (fmt, args);                                                 \
                printf ("\n");                                                      \
            }                                                                       \
            else if (a != b) {                                                      \
                fallout = true;                                                     \
                printf ("======== [VIDITH]: Hit on run: %d ========\n", run);       \
                printf (fmt, args) ;                                                \
                printf ("\n") ;                                                     \
                printf ("Jumbled? %d\n", A->jumbled) ;                              \
                printf ("Dumping [Ax, Ai]\n") ;                                     \
                int pi = 0;                                                         \
                int pval = A->p[pi];                                                \
                for (int i = 0; i < A->nvals; i++) {                                \
                    if (i == pval) {                                                \
                        printf ("== COL %d ==\n", pi);                              \
                        pi++;                                                       \
                        pval = A->p[pi];                                            \
                    }                                                               \
                    printf ("(%d): Ax: %0.5f; Ai: %ld\n", i,                        \
                        ((double*) A->x)[i], A->i[i]) ;                             \
                }                                                                   \
                printf ("Dumping Ap\n") ;                                           \
                for (int i = 0; i < A->plen + 1; i++) {                             \
                    printf ("(%d): Ap: %ld\n", i, A->p[i]) ;                        \
                }                                                                   \
                printf ("Ah exists? %d\n", (A->h != NULL)) ;                        \
                if (A->h != NULL) {                                                 \
                    printf ("Dumping Ah\n") ;                                       \
                    for (int i = 0; i < A->plen; i++) {                             \
                        printf ("(%d): Ah: %ld\n", i, A->h[i]) ;                    \
                    }                                                               \
                }                                                                   \
                printf ("Op is: %d\n", op->opcode) ;                                \
                printf ("Thunk is: %0.3f\n", *((double*) Thunk->x));                \
                printf ("======== [VIDITH]: Done ========\n") ;                     \
            }                                                                       \
        }

        // FIXME: Extract in-place handling out of this function
        info = GB_select_sparse (C, C_iso, op, flipij, A, ithunk, athunk,
            ythunk, Werk) ;
        
        if (compare) {
            // SAME (C->plen, T->plen, "hit plens: cpu: %ld, gpu: %ld", C->plen, T->plen) ;
            SAME (C->vlen, T->vlen, "hit vlen: cpu: %ld, gpu: %ld", C->vlen, T->vlen) ;
            SAME (C->vdim, T->vdim, "hit vdim: cpu: %ld, gpu: %ld", C->vdim, T->vdim) ;
            SAME (C->nvec, T->nvec, "hit nvec: cpu: %ld, gpu: %ld", C->nvec, T->nvec) ;
            SAME (C->nvec_nonempty, T->nvec_nonempty, "hit nvec_nonempty: cpu: %ld, gpu: %ld", C->nvec_nonempty, T->nvec_nonempty) ;
            SAME (C->nvals, T->nvals, "hit nvals: cpu: %ld, gpu: %ld", C->nvals, T->nvals) ;
            if (fallout) {
                exit(-1);
            }
        }
    }
 
    // FIXME: handle in_place_A case here, not in select_sparse:
    // transplant from T to either C (not in place) or A (in place)

    return (info) ;
}

