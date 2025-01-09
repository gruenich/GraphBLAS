//------------------------------------------------------------------------------
// GxB_Col_extract_32: w<M> = accum (w, A(I,j)) or A(j,I)', with 32-bit I
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Extract a single row or column from a matrix.  Note that in the
// GraphBLAS spec, row and column vectors are indistinguishable.  In this
// implementation, both are the same as an n-by-1 GrB_Matrix, except with
// restrictions on the matrix operations that can be performed on them.

#include "extract/GB_extract.h"
#include "mask/GB_get_mask.h"

GrB_Info GxB_Col_extract_32         // w<mask> = accum (w, A(I,j))
(
    GrB_Vector w,                   // input/output matrix for results
    const GrB_Vector mask,          // optional mask for w, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(w,t)
    const GrB_Matrix A,             // first input:  matrix A
    const uint32_t *I,              // row indices (32-bit)
    uint64_t ni,                    // number of row indices
    uint64_t j,                     // column index
    const GrB_Descriptor desc       // descriptor for w, mask, and A
)
{ 
GB_GOTCHA ; // this method will be deleted

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (w, mask, A,
        "GxB_Col_extract_32 (w, M, accum, A, I, ni, j, desc)") ;
    GB_RETURN_IF_NULL (w) ;
    GB_RETURN_IF_NULL (A) ;
    GB_BURBLE_START ("GrB_extract") ;

    ASSERT (GB_VECTOR_OK (w)) ;
    ASSERT (GB_IMPLIES (mask != NULL, GB_VECTOR_OK (mask))) ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        A_transpose, xx1, xx2, xx7) ;

    // get the mask
    GrB_Matrix M = GB_get_mask ((GrB_Matrix) mask, &Mask_comp, &Mask_struct) ;

    uint64_t ancols = (A_transpose ? GB_NROWS (A) : GB_NCOLS (A)) ;
    if (j >= ancols)
    { 
GB_GOTCHA ; // this method will be deleted
        GB_ERROR (GrB_INVALID_INDEX,
            "Column index j=" GBu " out of bounds; must be < " GBu ,
            j, ancols) ;
    }

    //--------------------------------------------------------------------------
    // extract the jth column (or jth row if A is transposed) using GB_extract
    //--------------------------------------------------------------------------

    // construct the column index list J = [ j ] of length nj = 1
    uint64_t J [1] ;
    J [0] = j ;

    //--------------------------------------------------------------------------
    // do the work in GB_extract
    //--------------------------------------------------------------------------

    info = GB_extract (
        (GrB_Matrix) w,    C_replace,   // w as a matrix, and descriptor
        M, Mask_comp, Mask_struct,      // mask and its descriptor
        accum,                          // optional accum for z=accum(w,t)
        A,                 A_transpose, // A and its descriptor
        I, true, ni,                    // row indices I and length ni (32-bit)
        J, false, 1,                    // one column index, nj = 1 (64-bit)
        Werk) ;

    GB_BURBLE_END ;
    return (info) ;
}

