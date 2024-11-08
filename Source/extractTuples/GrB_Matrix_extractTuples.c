//------------------------------------------------------------------------------
// GrB_Matrix_extractTuples: extract all tuples from a matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Extracts all tuples from a matrix, like [I,J,X] = find (A).  If any
// parameter I, J and/or X is NULL, then that component is not extracted.  The
// size of the I, J, and X arrays (those that are not NULL) is given by nvals,
// which must be at least as large as GrB_nvals (&nvals, A).  The values in the
// matrix are typecasted to the type of X, as needed.

// If any parameter I, J, and/or X is NULL, that component is not extracted.
// So to extract just the row and col indices, pass I and J as non-NULL,
// and X as NULL.  This is like [I,J,~] = find (A).

// If A is iso and X is not NULL, the iso scalar Ax [0] is expanded into X.

#include "GB.h"
#include "extractTuples/GB_extractTuples.h"

#define GB_EXTRACT(prefix,type,T,xtype)                                       \
GrB_Info GB_EVAL3 (prefix, _Matrix_extractTuples_, T)                         \
(                                                                             \
    GrB_Index *I,           /* array for returning row indices of tuples */   \
    GrB_Index *J,           /* array for returning col indices of tuples */   \
    type *X,                /* array for returning values of tuples      */   \
    GrB_Index *p_nvals,     /* I,J,X size on input; # tuples on output   */   \
    const GrB_Matrix A      /* matrix to extract tuples from             */   \
)                                                                             \
{                                                                             \
    GB_WHERE1 ("GrB_Matrix_extractTuples_" GB_STR(T) " (I, J, X, nvals, A)") ;\
    GB_BURBLE_START ("GrB_Matrix_extractTuples") ;                            \
    GB_RETURN_IF_NULL_OR_FAULTY (A) ;                                         \
    GB_RETURN_IF_NULL (p_nvals) ;                                             \
    GrB_Info info = GB_extractTuples (I, J, X, p_nvals, xtype, A, Werk) ;     \
    GB_BURBLE_END ;                                                           \
    GB_PRAGMA (omp flush)                                                     \
    return (info) ;                                                           \
}

//       prefix, C type of X, X code , X type
GB_EXTRACT (GrB, bool       , BOOL   , GrB_BOOL  )
GB_EXTRACT (GrB, int8_t     , INT8   , GrB_INT8  )
GB_EXTRACT (GrB, uint8_t    , UINT8  , GrB_UINT8 )
GB_EXTRACT (GrB, int16_t    , INT16  , GrB_INT16 )
GB_EXTRACT (GrB, uint16_t   , UINT16 , GrB_UINT16)
GB_EXTRACT (GrB, int32_t    , INT32  , GrB_INT32 )
GB_EXTRACT (GrB, uint32_t   , UINT32 , GrB_UINT32)
GB_EXTRACT (GrB, int64_t    , INT64  , GrB_INT64 )
GB_EXTRACT (GrB, uint64_t   , UINT64 , GrB_UINT64)
GB_EXTRACT (GrB, float      , FP32   , GrB_FP32  )
GB_EXTRACT (GrB, double     , FP64   , GrB_FP64  )
GB_EXTRACT (GxB, GxB_FC32_t , FC32   , GxB_FC32  )
GB_EXTRACT (GxB, GxB_FC64_t , FC64   , GxB_FC64  )
GB_EXTRACT (GrB, void       , UDT    , A->type   )

