//------------------------------------------------------------------------------
// GxB_Vector_build_32: build a sparse GraphBLAS vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// If dup is NULL: any duplicates result in an error.
// If dup is GxB_IGNORE_DUP: duplicates are ignored, which is not an error.
// If dup is a valid binary operator, it is used to reduce any duplicates to
// a single value.

#include "builder/GB_build.h"

#define GB_VECTOR_BUILD(function_name,ctype,xtype)                            \
GrB_Info function_name          /* build a vector from tuples */              \
(                                                                             \
    GrB_Vector w,               /* vector to build                    */      \
    const uint32_t *I,          /* array of row indices of tuples     */      \
    const ctype *X,             /* array of values of tuples          */      \
    uint64_t nvals,             /* number of tuples                   */      \
    const GrB_BinaryOp dup      /* binary op to assemble duplicates   */      \
)                                                                             \
{                                                                             \
    GB_WHERE (w, GB_STR(function_name) " (w, I, X, nvals, dup)") ;            \
    GB_BURBLE_START (GB_STR(function_name)) ;                                 \
    GB_RETURN_IF_NULL_OR_FAULTY (w) ;  /* check now so w->type can be done */ \
    ASSERT (GB_VECTOR_OK (w)) ;                                               \
    GrB_Info info = GB_build ((GrB_Matrix) w, I, NULL, X, nvals, dup, xtype,  \
        false, false, true, Werk) ;                                           \
    GB_BURBLE_END ;                                                           \
    return (info) ;                                                           \
}

// with 32-bit I arrays:
GB_VECTOR_BUILD (GxB_Vector_build_32_BOOL  , bool      , GrB_BOOL  )
GB_VECTOR_BUILD (GxB_Vector_build_32_INT8  , int8_t    , GrB_INT8  )
GB_VECTOR_BUILD (GxB_Vector_build_32_INT16 , int16_t   , GrB_INT16 )
GB_VECTOR_BUILD (GxB_Vector_build_32_INT32 , int32_t   , GrB_INT32 )
GB_VECTOR_BUILD (GxB_Vector_build_32_INT64 , int64_t   , GrB_INT64 )
GB_VECTOR_BUILD (GxB_Vector_build_32_UINT8 , uint8_t   , GrB_UINT8 )
GB_VECTOR_BUILD (GxB_Vector_build_32_UINT16, uint16_t  , GrB_UINT16)
GB_VECTOR_BUILD (GxB_Vector_build_32_UINT32, uint32_t  , GrB_UINT32)
GB_VECTOR_BUILD (GxB_Vector_build_32_UINT64, uint64_t  , GrB_UINT64)
GB_VECTOR_BUILD (GxB_Vector_build_32_FP32  , float     , GrB_FP32  )
GB_VECTOR_BUILD (GxB_Vector_build_32_FP64  , double    , GrB_FP64  )
GB_VECTOR_BUILD (GxB_Vector_build_32_FC32  , GxB_FC32_t, GxB_FC32  )
GB_VECTOR_BUILD (GxB_Vector_build_32_FC64  , GxB_FC64_t, GxB_FC64  )
GB_VECTOR_BUILD (GxB_Vector_build_32_UDT   , void      , w->type   )

