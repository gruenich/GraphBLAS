//------------------------------------------------------------------------------
// GB_subassign_15: C(I,J)<!M> += scalar ; using S
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// JIT: done.

// Method 15: C(I,J)<!M> += scalar ; using S

// M:           present
// Mask_struct: true or false
// Mask_comp:   true
// C_replace:   false
// accum:       present
// A:           scalar
// S:           constructed

// C: not bitmap, but can be full since no zombies are inserted in that case
// M: not bitmap

#include "assign/GB_subassign_methods.h"
#include "jitifyer/GB_stringify.h"
#define GB_FREE_ALL ;

GrB_Info GB_subassign_15
(
    GrB_Matrix C,
    // input:
    #define C_replace false
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
    #define Mask_comp true
    const bool Mask_struct,
    const GrB_BinaryOp accum,
    #define A NULL
    const void *scalar,
    const GrB_Type scalar_type,
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (!GB_IS_BITMAP (C)) ;
    ASSERT (!GB_any_aliased (C, M)) ;   // NO ALIAS of C==M

    GB_MATRIX_WAIT (M) ;

    //--------------------------------------------------------------------------
    // via the JIT or PreJIT kernel
    //--------------------------------------------------------------------------

    GrB_Info info = GB_subassign_jit (C,
        /* C_replace: */ false,
        I, ni, nI, Ikind, Icolon,
        J, nj, nJ, Jkind, Jcolon,
        M,
        /* Mask_comp: */ true,
        Mask_struct,
        accum,
        /* A: */ NULL,
        scalar, scalar_type,
        GB_SUBASSIGN, GB_JIT_KERNEL_SUBASSIGN_15, "subassign_15",
        Werk) ;
    if (info != GrB_NO_VALUE)
    { 
        return (info) ;
    }

    //--------------------------------------------------------------------------
    // via the generic kernel
    //--------------------------------------------------------------------------

    #define GB_GENERIC
    #define GB_SCALAR_ASSIGN 1
    #include "assign/include/GB_assign_shared_definitions.h"
    #include "assign/template/GB_subassign_15_template.c"
}

