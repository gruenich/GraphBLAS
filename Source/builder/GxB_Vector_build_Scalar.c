//------------------------------------------------------------------------------
// GxB_Vector_build_Scalar: build a sparse GraphBLAS vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// GxB_Vector_build_Scalar builds a vector w whose values in its sparsity
// pattern are all equal to a value given by a GrB_Scalar.  Unlike the
// GrB_Vector_build_* methods, there is no binary dup operator.  Instead, any
// duplicate indices are ignored, which is not an error condition.  The I array
// is of size nvals, just like GrB_Vector_build_*.

#include "builder/GB_build.h"
#define GB_FREE_ALL ;

GrB_Info GxB_Vector_build_Scalar    // build a vector from (i,scalar) tuples
(
    GrB_Vector w,                   // vector to build
    const uint64_t *I,              // array of row indices of tuples
    GrB_Scalar scalar,              // value for all tuples
    uint64_t nvals                  // number of tuples
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE (w, "GxB_Vector_build_Scalar (w, I, scalar, nvals)") ;
    GB_BURBLE_START ("GxB_Vector_build_Scalar") ;
    GB_RETURN_IF_NULL_OR_FAULTY (w) ;
    GB_RETURN_IF_NULL_OR_FAULTY (scalar) ;
    ASSERT (GB_VECTOR_OK (w)) ;
    GB_MATRIX_WAIT (scalar) ;
    if (GB_nnz ((GrB_Matrix) scalar) != 1)
    { 
        GB_ERROR (GrB_EMPTY_OBJECT, "Scalar value is %s", "missing") ;
    }

    //--------------------------------------------------------------------------
    // build the vector
    //--------------------------------------------------------------------------

    GrB_Info info = GB_build ((GrB_Matrix) w, I, NULL, scalar->x, nvals,
        GxB_IGNORE_DUP, scalar->type, false, true, false, Werk) ;
    GB_BURBLE_END ;
    return (info) ;
}

