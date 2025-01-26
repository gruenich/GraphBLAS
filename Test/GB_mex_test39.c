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
    GB_free_memory (&X4, X4_size) ;            \
    if (X2 != NULL) mxFree (X2) ;       \
    X2 = NULL ;                         \
    GxB_Container_free (&Container) ;   \
    GrB_Vector_free (&V) ;              \
    GrB_Vector_free (&A) ;              \
    GrB_Vector_free (&C) ;              \
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
    GrB_Vector V = NULL ;
    GrB_Matrix A = NULL, C = NULL ;
    GxB_Container Container = NULL ;
    uint32_t *X = NULL, *X2 = NULL, *X3 = NULL, *X4 = NULL, *X5 = NULL ;
    bool malloc_debug = GB_mx_get_global (true) ;
    uint64_t n = 10, n2 = 999, X_size, X_size2 = 911, n4 = 0, X4_size = 0,
        n5 = 0, X5_size = 0 ;
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

    int expected = GrB_INVALID_VALUE ;
    ERR (GxB_Vector_load (V, &X, n, 2, GrB_UINT32, false, NULL)) ;

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

    expected = GrB_INVALID_OBJECT ;
    OK (GrB_Vector_free (&V)) ;
    OK (GrB_Vector_new (&V, GrB_FP64, n)) ;
    ERR (GxB_Vector_unload (V, &X5, &n5, &X5_size, &type, &read_only, NULL)) ;
    OK (GrB_Vector_free (&V)) ;

    //--------------------------------------------------------------------------
    // test the Container with a matrix
    //--------------------------------------------------------------------------

    printf ("\n------------------- testing Container unload (Matrix):\n") ;
    OK (GrB_Matrix_new (&A, GrB_FP64, n, n)) ;
    for (int i = 0 ; i < n ; i++)
    {
        double x = 2*i + 0.1 ;
        OK (GrB_Matrix_setElement_FP64 (A, x, i, i)) ;
    }
    OK (GxB_print (A, 5)) ;

    OK (GxB_Container_new (&Container)) ;
    OK (GxB_unload_Matrix_into_Container (A, Container, NULL)) ;
    OK (GxB_print (A, 5)) ;

    printf ("\n------------------- testing Container load (Matrix):\n") ;
    OK (GxB_load_Matrix_from_Container (A, Container, NULL)) ;
    OK (GxB_print (A, 5)) ;

    //--------------------------------------------------------------------------
    // test the Container with a vector
    //--------------------------------------------------------------------------

    printf ("\n------------------- testing Container unload (Matrix):\n") ;
    OK (GrB_Vector_new (&V, GrB_FP64, n)) ;
    for (int i = 0 ; i < n/2 ; i++)
    {
        double x = 2*i + 0.1 ;
        OK (GrB_Vector_setElement_FP64 (V, x, i)) ;
    }
    OK (GxB_print (V, 5)) ;

    OK (GxB_unload_Vector_into_Container (V, Container, NULL)) ;
    OK (GxB_print (V, 5)) ;

    printf ("\n------------------- testing Container load (Vector):\n") ;
    OK (GxB_load_Matrix_from_Container (V, Container, NULL)) ;
    OK (GxB_print (V, 5)) ;

    //--------------------------------------------------------------------------
    // test extract with empty vectors
    //--------------------------------------------------------------------------

    OK (GrB_Matrix_new (&C, GrB_FP64, 0, 0)) ;
    OK (GrB_Vector_clear (V)) ;
    #define GET_DEEP_COPY ;
    #define FREE_DEEP_COPY ;
    METHOD (GxB_Matrix_extract_Vector_(C, NULL, NULL, A, V, V, NULL)) ;
    OK (GxB_print (C, 5)) ;

    //--------------------------------------------------------------------------
    // finalize GraphBLAS
    //--------------------------------------------------------------------------

    FREE_ALL ;
    GB_mx_put_global (true) ;
    printf ("\nGB_mex_test39:  all tests passed\n\n") ;
}

