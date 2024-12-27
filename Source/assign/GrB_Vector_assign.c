//------------------------------------------------------------------------------
// GrB_Vector_assign: w<M>(Rows) = accum (w(Rows),u)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Compare with GxB_Vector_subassign, which uses M and C_replace differently

// DONE: 32/64 bit
#define GB_DEBUG

#include "assign/GB_assign.h"
#include "assign/GB_bitmap_assign.h"
#include "mask/GB_get_mask.h"

GrB_Info GrB_Vector_assign          // w<M>(Rows) = accum (w(Rows),u)
(
    GrB_Vector w,                   // input/output matrix for results
    const GrB_Vector M_in,          // optional mask for w, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(w(Rows),t)
    const GrB_Vector u,             // first input:  vector u
    const uint64_t *Rows,           // row indices
    uint64_t nRows,                 // number of row indices
    const GrB_Descriptor desc       // descriptor for w and M
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (w, M_in, u,
        "GrB_Vector_assign (w, M, accum, u, Rows, nRows, desc)") ;
    GB_RETURN_IF_NULL (w) ;
    GB_RETURN_IF_NULL (u) ;
    GB_BURBLE_START ("GrB_assign") ;

    ASSERT (GB_VECTOR_OK (w)) ;
    ASSERT (M_in == NULL || GB_VECTOR_OK (M_in)) ;
    ASSERT (GB_VECTOR_OK (u)) ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        xx1, xx2, xx3, xx7) ;

    // get the mask
    GrB_Matrix M = GB_get_mask ((GrB_Matrix) M_in, &Mask_comp, &Mask_struct) ;

    //--------------------------------------------------------------------------
    // w(Rows)<M> = accum (w(Rows), u) and variations
    //--------------------------------------------------------------------------

    info = GB_assign (
        (GrB_Matrix) w, C_replace,      // w vector and its descriptor
        M, Mask_comp, Mask_struct,      // mask and its descriptor
        false,                          // do not transpose the mask
        accum,                          // for accum (C(Rows,:),A)
        (GrB_Matrix) u, false,          // u as a matrix; never transposed
        Rows, false, nRows,             // row indices
        GrB_ALL, false, 1,              // all column indices
        false, NULL, GB_ignore_code,    // no scalar expansion
        GB_ASSIGN,
        Werk) ;

    GB_BURBLE_END ;
    return (info) ;
}

