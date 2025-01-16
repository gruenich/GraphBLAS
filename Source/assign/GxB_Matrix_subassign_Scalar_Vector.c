//------------------------------------------------------------------------------
// GxB_Matrix_subassign_Scalar_Vector: assign a scalar to matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "assign/GB_subassign.h"
#include "ij/GB_ij.h"
#define GB_FREE_ALL                             \
    if (I_size > 0) GB_FREE (&I, I_size) ;      \
    if (J_size > 0) GB_FREE (&J, J_size) ;

GrB_Info GxB_Matrix_subassign_Scalar_Vector   // C(I,J)<Mask> = accum (C(I,J),x)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix Mask,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C(I,J),x)
    const GrB_Scalar scalar,        // scalar to assign to C(I,J)
    const GrB_Vector I_vector,      // row indices
    const GrB_Vector J_vector,      // column indices
    const GrB_Descriptor desc       // descriptor for C(I,J) and Mask
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (C, Mask, scalar,
        "GxB_Matrix_subassign_Scalar_Vector (C, M, accum, s, I, J, desc)") ;
    GB_BURBLE_START ("GxB_Matrix_subassign_Scalar_Vector") ;

    //--------------------------------------------------------------------------
    // get the index vectors
    //--------------------------------------------------------------------------

    void *I = NULL, *J = NULL ;
    size_t I_size = 0, J_size = 0 ;
    int64_t ni = 0, nj = 0 ;
    GrB_Type I_type = NULL, J_type = NULL ;
    GB_OK (GB_ijvector (I_vector, false, 0, desc, false,
        &I, &ni, &I_size, &I_type, Werk)) ;
    GB_OK (GB_ijvector (J_vector, false, 1, desc, false,
        &J, &nj, &J_size, &J_type, Werk)) ;
    bool I_is_32 = (I_type == GrB_UINT32) ;
    bool J_is_32 = (J_type == GrB_UINT32) ;

    //--------------------------------------------------------------------------
    // C<M>(I,J) = accum (C(I,J), scalar)
    //--------------------------------------------------------------------------

    GB_OK (GB_Matrix_subassign_scalar (C, Mask, accum, scalar,
        I, I_is_32, ni, J, J_is_32, nj, desc, Werk)) ;

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    GB_FREE_ALL ;
    GB_BURBLE_END ;
    return (GrB_SUCCESS) ;
}

