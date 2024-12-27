//------------------------------------------------------------------------------
// GxB_Col_subassign: C(Rows,col)<M> = accum (C(Rows,col),u)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Compare with GrB_Col_assign, which uses M and C_replace differently

#include "assign/GB_subassign.h"
#include "mask/GB_get_mask.h"

GrB_Info GxB_Col_subassign          // C(Rows,col)<M> = accum (C(Rows,col),u)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Vector M_in,          // mask for C(Rows,col), unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(C(Rows,col),t)
    const GrB_Vector u,             // input vector
    const uint64_t *Rows,           // row indices
    uint64_t nRows,                 // number of row indices
    uint64_t col,                   // column index
    const GrB_Descriptor desc       // descriptor for C(Rows,col) and M
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (C, M_in, u,
        "GxB_Col_subassign (C, M, accum, u, Rows, nRows, col, desc)") ;
    GB_RETURN_IF_NULL (C) ;
    GB_RETURN_IF_NULL (u) ;
    GB_BURBLE_START ("GxB_subassign") ;

    ASSERT (M_in == NULL || GB_VECTOR_OK (M_in)) ;
    ASSERT (GB_VECTOR_OK (u)) ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        xx1, xx2, xx3, xx7) ;

    // get the mask
    GrB_Matrix M = GB_get_mask ((GrB_Matrix) M_in, &Mask_comp, &Mask_struct) ;

    //--------------------------------------------------------------------------
    // C(Rows,col)<M> = accum (C(Rows,col), u) and variations
    //--------------------------------------------------------------------------

    // construct the column index list Cols = [ col ] of length nCols = 1
    uint64_t Cols [1] ;
    Cols [0] = col ;

    info = GB_subassign (
        C, C_replace,                   // C matrix and its descriptor
        M, Mask_comp, Mask_struct,      // mask and its descriptor
        false,                          // do not transpose the mask
        accum,                          // for accum (C(Rows,col),u)
        (GrB_Matrix) u, false,          // u as a matrix; never transposed
        Rows, false, nRows,             // row indices
        Cols, false, 1,                 // a single column index
        false, NULL, GB_ignore_code,    // no scalar expansion
        Werk) ;

    GB_BURBLE_END ;
    return (info) ;
}

