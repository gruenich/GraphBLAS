//------------------------------------------------------------------------------
// GrB_Matrix_eWiseAdd: matrix element-wise operations, set union
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// C<M> = accum (C,A+B) and variations.

#include "ewise/GB_ewise.h"
#include "mask/GB_get_mask.h"

#define GB_EWISE(op)                                                        \
    /* check inputs */                                                      \
    GB_RETURN_IF_NULL (C) ;                                                 \
    GB_RETURN_IF_NULL (A) ;                                                 \
    GB_RETURN_IF_NULL (B) ;                                                 \
    GB_WHERE4 (C, M_in, A, B,                                               \
        "GrB_Matrix_eWiseAdd (C, M, accum, op, A, B, desc)") ;              \
    GB_BURBLE_START ("GrB_eWiseAdd") ;                                      \
    /* get the descriptor */                                                \
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,       \
        A_tran, B_tran, xx, xx7) ;                                          \
    /* get the mask */                                                      \
    GrB_Matrix M = GB_get_mask (M_in, &Mask_comp, &Mask_struct) ;           \
    /* C<M> = accum (C,T) where T = A+B, A'+B, A+B', or A'+B' */            \
    info = GB_ewise (                                                       \
        C,              C_replace,  /* C and its descriptor        */       \
        M, Mask_comp, Mask_struct,  /* mask and its descriptor     */       \
        accum,                      /* accumulate operator         */       \
        op,                         /* operator that defines '+'   */       \
        A,              A_tran,     /* A matrix and its descriptor */       \
        B,              B_tran,     /* B matrix and its descriptor */       \
        true,                       /* eWiseAdd                    */       \
        false, NULL, NULL,          /* not eWiseUnion              */       \
        Werk) ;                                                             \
    GB_BURBLE_END ;

//------------------------------------------------------------------------------
// GrB_Matrix_eWiseAdd_BinaryOp: matrix addition
//------------------------------------------------------------------------------

GrB_Info GrB_Matrix_eWiseAdd_BinaryOp       // C<M> = accum (C, A+B)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix M_in,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C,T)
    const GrB_BinaryOp op,          // defines '+' for T=A+B
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Matrix B,             // second input: matrix B
    const GrB_Descriptor desc       // descriptor for C, M, A, and B
)
{ 
    GB_RETURN_IF_NULL_OR_FAULTY (op) ;
    GB_EWISE (op) ;
    return (info) ;
}

//------------------------------------------------------------------------------
// GrB_Matrix_eWiseAdd_Monoid: matrix addition
//------------------------------------------------------------------------------

// C<M> = accum (C,A+B) and variations.

GrB_Info GrB_Matrix_eWiseAdd_Monoid         // C<M> = accum (C, A+B)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix M_in,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C,T)
    const GrB_Monoid monoid,        // defines '+' for T=A+B
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Matrix B,             // second input: matrix B
    const GrB_Descriptor desc       // descriptor for C, M, A, and B
)
{ 
    GB_RETURN_IF_NULL_OR_FAULTY (monoid) ;
    GB_EWISE (monoid->op) ;
    return (info) ;
}

//------------------------------------------------------------------------------
// GrB_Matrix_eWiseAdd_Semiring: matrix addition
//------------------------------------------------------------------------------

// C<M> = accum (C,A+B) and variations.

GrB_Info GrB_Matrix_eWiseAdd_Semiring       // C<M> = accum (C, A+B)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix M_in,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C,T)
    const GrB_Semiring semiring,    // defines '+' for T=A+B
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Matrix B,             // second input: matrix B
    const GrB_Descriptor desc       // descriptor for C, M, A, and B
)
{ 
    GB_RETURN_IF_NULL_OR_FAULTY (semiring) ;
    GB_EWISE (semiring->add->op) ;
    return (info) ;
}

