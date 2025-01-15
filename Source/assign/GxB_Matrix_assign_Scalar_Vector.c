//------------------------------------------------------------------------------
// GxB_Matrix_assign_Scalar_Vector: assign a scalar to matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "assign/GB_assign.h"
#include "ij/GB_ij.h"
#define GB_FREE_ALL                             \
    if (I_size > 0) GB_FREE (&I, I_size) ;      \
    if (J_size > 0) GB_FREE (&J, J_size) ;

GrB_Info GxB_Matrix_assign_Scalar_Vector   // C<Mask>(I,J) = accum (C(I,J),x)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix Mask,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C(I,J),x)
    const GrB_Scalar scalar,        // scalar to assign to C(I,J)
    const GrB_Vector I_vector,      // row indices
    const GrB_Vector J_vector,      // column indices
    const GrB_Descriptor desc       // descriptor for C and Mask
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE3 (C, Mask, scalar,
        "GxB_Matrix_assign_Scalar_Vector (C, M, accum, s, I, J, desc)") ;
    GB_BURBLE_START ("GxB_Matrix_assign_Scalar_Vector") ;

    //--------------------------------------------------------------------------
    // get the index vectors
    //--------------------------------------------------------------------------

    void *I = NULL, *J = NULL ;
    size_t I_size = 0, J_size = 0 ;
    int64_t ni = 0, nj = 0 ;
    bool I_is_32 = false, J_is_32 = false ;
    GB_OK (GB_ijvector (I_vector, false, 0, desc, &I, &I_is_32, &ni, &I_size,
        Werk)) ;
    GB_OK (GB_ijvector (J_vector, false, 1, desc, &J, &J_is_32, &nj, &J_size,
        Werk)) ;

    //--------------------------------------------------------------------------
    // C<M>(I,J) = accum (C(I,J), scalar)
    //--------------------------------------------------------------------------

    GB_OK (GB_Matrix_assign_scalar (C, Mask, accum, scalar,
        I, I_is_32, ni, J, J_is_32, nj, desc, Werk)) ;

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    GB_FREE_ALL ;
    GB_BURBLE_END ;
    return (GrB_SUCCESS) ;
}

