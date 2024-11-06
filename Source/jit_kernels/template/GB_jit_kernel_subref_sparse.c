//------------------------------------------------------------------------------
// GB_jit_kernel_subref_sparse.c: A = C(I,J) where C and A are sparse/hyper
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "include/GB_subref_method.h"

GB_JIT_GLOBAL GB_JIT_KERNEL_SUBREF_SPARSE_PROTO (GB_jit_kernel) ;
GB_JIT_GLOBAL GB_JIT_KERNEL_SUBREF_SPARSE_PROTO (GB_jit_kernel)
{
    // get callback functions
    GB_GET_CALLBACK (GB_qsort_1b) ;

    #define GB_PHASE_2_OF_2
    const int64_t *restrict Cp = C->p ;
    int64_t *restrict Ci = C->i ;
    #define GB_CSIZE1 1
    #define GB_CSIZE2 sizeof (GB_C_TYPE)
    #define GB_COPY_RANGE(pC,pA,len) \
        memcpy (Cx + (pC), Ax + (pA), (len) * sizeof (GB_C_TYPE)) ;
    #define GB_COPY_ENTRY(pC,pA) Cx [pC] = Ax [pA] ;
    const GB_C_TYPE *restrict Ax = (GB_C_TYPE *) A->x ;
          GB_C_TYPE *restrict Cx = (GB_C_TYPE *) C->x ;
    #include "template/GB_subref_template.c"
}

