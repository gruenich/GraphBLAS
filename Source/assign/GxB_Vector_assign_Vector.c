//------------------------------------------------------------------------------
// GxB_Vector_assign_Vector: w<M>(Rows) = accum (w(Rows),u)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Compare with GxB_Vector_subassign, which uses M and C_replace differently

#include "assign/GB_assign.h"
#include "assign/GB_bitmap_assign.h"
#include "mask/GB_get_mask.h"
#include "ij/GB_ij.h"
#define GB_FREE_ALL                             \
    if (I_size > 0) GB_FREE (&I, I_size) ;

GrB_Info GxB_Vector_assign_Vector   // w<mask>(I) = accum (w(I),u)
(
    GrB_Vector w,                   // input/output matrix for results
    const GrB_Vector mask,          // optional mask for w, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(w(I),t)
    const GrB_Vector u,             // first input:  vector u
    const GrB_Vector I_vector,      // row indices
    const GrB_Descriptor desc       // descriptor for w and mask
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (w, mask, u,
        "GxB_Vector_assign_Vectoe (w, M, accum, u, I, desc)") ;
    GB_RETURN_IF_NULL (w) ;
    GB_RETURN_IF_NULL (u) ;
    GB_BURBLE_START ("GrB_assign") ;

    ASSERT (GB_VECTOR_OK (w)) ;
    ASSERT (mask == NULL || GB_VECTOR_OK (mask)) ;
    ASSERT (GB_VECTOR_OK (u)) ;

    void *I = NULL ;
    size_t I_size = 0 ;
    int64_t ni = 0 ;
    bool I_is_32 = false ;

    // FIXME: get Row_values and Col_values from the descriptor

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, Mask_struct,
        xx1, xx2, xx3, xx7) ;

    // get the mask
    GrB_Matrix M = GB_get_mask ((GrB_Matrix) mask, &Mask_comp, &Mask_struct) ;

    //--------------------------------------------------------------------------
    // get the index vector
    //--------------------------------------------------------------------------

    GB_OK (GB_ijvector (I_vector, true, (w == I_vector),
        &I, &I_is_32, &ni, &I_size, Werk)) ;

    //--------------------------------------------------------------------------
    // w(I)<M> = accum (w(I), u) and variations
    //--------------------------------------------------------------------------

    info = GB_assign (
        (GrB_Matrix) w, C_replace,      // w vector and its descriptor
        M, Mask_comp, Mask_struct,      // mask and its descriptor
        false,                          // do not transpose the mask
        accum,                          // for accum (C(I,:),A)
        (GrB_Matrix) u, false,          // u as a matrix; never transposed
        I, I_is_32, ni,                 // row indices
        GrB_ALL, false, 1,              // all column indices
        false, NULL, GB_ignore_code,    // no scalar expansion
        GB_ASSIGN,
        Werk) ;

    if (info == GrB_SUCCESS)
    {
        ASSERT_MATRIX_OK ((GrB_Matrix) w, "w result, GrB_Vector_assign", GB0) ;
        GB_OK (GB_valid_matrix ((GrB_Matrix) w)) ;
    }

    GB_BURBLE_END ;
    GB_FREE_ALL ;
    return (info) ;
}

