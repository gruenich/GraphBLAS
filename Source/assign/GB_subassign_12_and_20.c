//------------------------------------------------------------------------------
// GB_subassign_12_and_20: C(I,J)<M or !M,repl> += A ; using S
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// JIT: done.

// Method 12: C(I,J)<M,repl> += A ; using S
// Method 20: C(I,J)<!M,repl> += A ; using S

// M:           present
// Mask_stuct:  true or false
// Mask_comp:   true or false
// C_replace:   true
// accum:       present
// A:           matrix
// S:           constructed

// C: not bitmap: use GB_bitmap_assign instead
// M, A: any sparsity structure.

#include "assign/GB_subassign_methods.h"
#include "jitifyer/GB_stringify.h"
#define GB_FREE_ALL ;

GrB_Info GB_subassign_12_and_20
(
    GrB_Matrix C,
    // input:
    const GrB_Index *I,
    const int64_t ni,
    const int64_t nI,
    const int Ikind,
    const int64_t Icolon [3],
    const GrB_Index *J,
    const int64_t nj,
    const int64_t nJ,
    const int Jkind,
    const int64_t Jcolon [3],
    const GrB_Matrix M,
    const bool Mask_struct,         // if true, use the only structure of M
    const bool Mask_comp,           // if true, !M, else use M
    const GrB_BinaryOp accum,
    const GrB_Matrix A,
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (!GB_IS_BITMAP (C)) ; ASSERT (!GB_IS_FULL (C)) ;
    ASSERT (!GB_any_aliased (C, M)) ;   // NO ALIAS of C==M
    ASSERT (!GB_any_aliased (C, A)) ;   // NO ALIAS of C==A

    GB_UNJUMBLE (M) ;
    GB_UNJUMBLE (A) ;

    //--------------------------------------------------------------------------
    // via the JIT or PreJIT kernel
    //--------------------------------------------------------------------------

    GrB_Info info = GB_subassign_jit (C,
        /* C_replace: */ true,
        I, ni, nI, Ikind, Icolon,
        J, nj, nJ, Jkind, Jcolon,
        M,
        Mask_comp,
        Mask_struct,
        accum,
        A,
        /* scalar, scalar_type: */ NULL, NULL,
        GB_SUBASSIGN, GB_JIT_KERNEL_SUBASSIGN_12, "subassign_12",
        Werk) ;
    if (info != GrB_NO_VALUE)
    { 
        return (info) ;
    }

    //--------------------------------------------------------------------------
    // via the generic kernel
    //--------------------------------------------------------------------------

    #define GB_GENERIC
    #define GB_SCALAR_ASSIGN 0
    #include "assign/include/GB_assign_shared_definitions.h"
    #include "assign/template/GB_subassign_12_template.c"
}

