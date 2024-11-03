//------------------------------------------------------------------------------
// GB_bitmap_assign_6b_whole:  C = A where C is bitmap and A is sparse/hyper
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// JIT: needed.

#include "assign/GB_bitmap_assign_methods.h"
#include "assign/GB_subassign_dense.h"
#define GB_GENERIC
#include "assign/include/GB_assign_shared_definitions.h"

#undef  GB_FREE_ALL
#define GB_FREE_ALL ;

GrB_Info GB_bitmap_assign_6b_whole  // C bitmap, no M, no accum
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    #define C_replace false
    #define I NULL              /* I index list */
    #define ni 0
    #define nI 0
    #define Ikind GB_ALL
    #define Icolon NULL
    #define J NULL              /* J index list */
    #define nj 0
    #define nJ 0
    #define Jkind GB_ALL
    #define Jcolon NULL
    #define M NULL              /* mask matrix, not present here */
    #define Mask_comp false
    #define Mask_struct true
    #define accum NULL          /* not present */
    const GrB_Matrix A,         // input matrix, not transposed
    #define scalar NULL
    #define scalar_type NULL
    #define assign_kind         GB_ASSIGN
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (GB_IS_BITMAP (C)) ;
    ASSERT (GB_IS_HYPERSPARSE (A) || GB_IS_SPARSE (A)) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign_6b_whole", GB0) ;
    ASSERT_MATRIX_OK (A, "A for bitmap assign_6b_whole", GB0) ;

    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    #undef  GB_FREE_ALL
    #define GB_FREE_ALL GB_FREE_ALL_FOR_BITMAP
    GB_GET_C_A_SCALAR_FOR_BITMAP

    //--------------------------------------------------------------------------
    // C = A, where C is bitmap and A is sparse/hyper
    //--------------------------------------------------------------------------

    GB_memset (Cb, 0, cnzmax, nthreads_max) ;
    cnvals = 0 ;
    #define GB_AIJ_WORK(pC,pA)                              \
    {                                                       \
        /* Cx [pC] = Ax [pA] */                             \
        GB_COPY_aij_to_C (Cx,pC,Ax,pA,A_iso,cwork,C_iso) ;  \
        Cb [pC] = 1 ;                                       \
    }
    #include "template/GB_bitmap_assign_A_whole_template.c"
    GB_A_NVALS (anz) ;
    C->nvals = anz ;

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    GB_FREE_ALL ;
    ASSERT_MATRIX_OK (C, "final C bitmap assign_6b_whole", GB0) ;
    return (GrB_SUCCESS) ;
}

