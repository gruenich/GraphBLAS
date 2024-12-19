//------------------------------------------------------------------------------
// GrB_Vector_new: create a new vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// The new vector is n-by-1, with no entries in it.
// A->p is size 2 and all zero.  Contents A->x and A->i are NULL.
// If this method fails, *v is set to NULL.  Vectors are not hypersparse,
// so format is standard CSC, and A->h is NULL.

#include "GB.h"

#define GB_FREE_ALL GB_Matrix_free ((GrB_Matrix *) v) ;

GrB_Info GrB_Vector_new     // create a new vector with no entries
(
    GrB_Vector *v,          // handle of vector to create
    GrB_Type type,          // type of vector to create
    uint64_t n              // dimension is n-by-1
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    GB_CHECK_INIT ;
    GB_RETURN_IF_NULL (v) ;
    (*v) = NULL ;
    GB_RETURN_IF_NULL_OR_FAULTY (type) ;

    if (n > GB_NMAX)
    { 
        // problem too large
        return (GrB_INVALID_VALUE) ;
    }

    //--------------------------------------------------------------------------
    // create the vector
    //--------------------------------------------------------------------------

    int64_t vlen = (int64_t) n ;

#if 0
    // get global pi_control
    int8_t p_control = GB_Global_p_control_get ( ) ;
    int8_t i_control = GB_Global_i_control_get ( ) ;
#else
    // HACK for now:
    int8_t p_control = GxB_PREFER_32_BITS ;
    int8_t i_control = GxB_PREFER_32_BITS ;
#endif

    // determine the p_is_32 and i_is_32 settings for the new vector
    bool Vp_is_32, Vi_is_32 ;
    GB_OK (GB_determine_pi_is_32 (&Vp_is_32, &Vi_is_32, p_control, i_control,
        GxB_SPARSE, 1, vlen, 1, true)) ;

    GB_OK (GB_new ((GrB_Matrix *) v, // new user header
        type, vlen, 1, GB_ph_calloc,
        true,  // a GrB_Vector is always held by-column
        GxB_SPARSE, GB_Global_hyper_switch_get ( ), 1,
        Vp_is_32, Vi_is_32)) ;

    // HACK for now:
    ASSERT_VECTOR_OK (*v, "GrB_Vector_new before convert", GB0) ;
    GB_OK (GB_convert_int (*v, false, false, true)) ; // FIXME
    ASSERT_VECTOR_OK (*v, "GrB_Vector_new after convert", GB0) ;
    GB_OK (GB_valid_matrix ((GrB_Matrix *) *v)) ;

    return (GrB_SUCCESS) ;
}

