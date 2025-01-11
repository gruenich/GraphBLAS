//------------------------------------------------------------------------------
// GB_mex_test39: load/unload
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_mex.h"
#include "GB_mex_errors.h"

#undef  FREE_ALL
#define FREE_ALL                        \
{                                       \
    GB_FREE (&X4, X4_size) ;            \
    if (X2 != NULL) mxFree (X2) ;       \
    X2 = NULL ;                         \
    GrB_Matrix_free (&V) ;              \
}

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    //--------------------------------------------------------------------------
    // startup GraphBLAS
    //--------------------------------------------------------------------------

    GrB_Info info ;
    GrB_Matrix V = NULL ;
    uint32_t *X = NULL, *X2 = NULL, *X3 = NULL, *X4 = NULL ;
    bool malloc_debug = GB_mx_get_global (true) ;
    uint64_t n = 10, n2 = 999, X_size, X_size2 = 911, n4 = 0, X4_size = 0 ;
    GrB_Type type = NULL ;
    bool read_only = false ;

    //--------------------------------------------------------------------------
    // test load/unload
    //--------------------------------------------------------------------------

    X_size = GB_IMAX (1, n * sizeof (uint32_t)) ;
    X = mxMalloc (X_size) ;
    X2 = X ;

    OK (GrB_Vector_new (&V, GrB_FP64, 0)) ;
    OK (GxB_print (V, 5)) ;

    for (int64_t i = 0 ; i < n ; i++)
    {
        X [i] = i ;
    }

    OK (GxB_Vector_load (V, &X, n, X_size, GrB_UINT32, false, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (X == NULL) ;

    OK (GxB_Vector_unload (V, &X3, &n2, &X_size2, &type, &read_only, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (X3 == X2) ;
    CHECK (n2 == n) ;
    CHECK (X_size == X_size2) ;
    CHECK (type == GrB_UINT32) ;
    CHECK (!read_only) ;

    for (int64_t i = 0 ; i < n ; i++)
    {
        CHECK (X3 [i] == i) ;
    }

    OK (GxB_Vector_unload (V, &X3, &n2, &X_size2, &type, &read_only, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (X3 == NULL) ;
    CHECK (n2 == 0) ;
    CHECK (X_size2 == 0) ;
    CHECK (type == GrB_UINT32) ;
    CHECK (!read_only) ;

    OK (GrB_Vector_free (&V)) ;
    OK (GrB_Vector_new (&V, GrB_UINT32, n)) ;
    OK (GrB_set (V, GxB_SPARSE, GxB_SPARSITY_CONTROL)) ;
    for (int64_t i = 0 ; i < n ; i++)
    {
        OK (GrB_Vector_setElement_UINT32 (V, 2*i, i)) ;
    }

//  OK (GxB_print (V, 5)) ;
//  OK (GrB_wait (V, GrB_MATERIALIZE)) ;

    OK (GxB_print (V, 5)) ;
    OK (GxB_Vector_unload (V, &X4, &n4, &X4_size, &type, &read_only, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (n4 == n) ;
    CHECK (X4 != NULL) ;
    for (int64_t i = 0 ; i < n ; i++)
    {
        CHECK (X4 [i] == 2*i) ;
    }

    //--------------------------------------------------------------------------
    // finalize GraphBLAS
    //--------------------------------------------------------------------------

    FREE_ALL ;
    GB_mx_put_global (true) ;
    printf ("\nGB_mex_test39:  all tests passed\n\n") ;
}

