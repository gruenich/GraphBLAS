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
    if (X4 != NULL) mxFree (X4) ;       \
    X3 = NULL ;                         \
    if (X != NULL) mxFree (X) ;         \
    X = NULL ;                          \
    GxB_Container_free (&Container) ;   \
    GrB_Vector_free (&V) ;              \
    GrB_Matrix_free (&A) ;              \
    GrB_Matrix_free (&C) ;              \
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
    X = mxMalloc (X_size) ;     // X is owned by the user application
    X2 = X ;

    // add X to the debug memtable, so GxB_Vector_unload can find it in the
    // table and remove it.  This is not required; just for debugging.
    printf ("mxMalloc: X = %p\n", (void *) X) ;
    GB_Global_memtable_add (X, X_size) ;

    OK (GrB_Vector_new (&V, GrB_FP64, 0)) ;
    OK (GxB_print (V, 5)) ;

    for (int64_t i = 0 ; i < n ; i++)
    {
        X [i] = i ;
    }

    int expected = GrB_INVALID_VALUE ;
    ERR (GxB_Vector_load (V, (void **) &X, GrB_UINT32, n, 2, false, NULL)) ;
    CHECK (X == X2) ;           // X is still owned by the user application

    OK (GxB_Vector_load (V, (void **) &X, GrB_UINT32, n, X_size, false, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (X == NULL) ;         // X is not freed, but owned by V
    CHECK (X2 != NULL) ;        // X2 is not owned by the user application

    OK (GxB_Vector_unload (V, (void **) &X, &type, &n2, &X_size2, &read_only, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (X == X2) ;           // X is owned by the user application again
    CHECK (n2 == n) ;
    CHECK (X_size == X_size2) ;
    CHECK (type == GrB_UINT32) ;
    CHECK (!read_only) ;

    for (int64_t i = 0 ; i < n ; i++)
    {
        CHECK (X [i] == i) ;
    }

    // unload an empty vector
    OK (GxB_Vector_unload (V, (void **) &X3, &type, &n2, &X_size2, &read_only, NULL)) ;
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

    OK (GxB_print (V, 5)) ;
    OK (GxB_Vector_unload (V, (void **) &X4, &type, &n4, &X4_size, &read_only, NULL)) ;
    OK (GxB_print (V, 5)) ;
    CHECK (n4 == n) ;
    CHECK (X4 != NULL) ;            // X4 is owned by the user application
    for (int64_t i = 0 ; i < n ; i++)
    {
        CHECK (X4 [i] == 2*i) ;
    }

    expected = GrB_INVALID_OBJECT ;
    OK (GrB_Vector_free (&V)) ;
    OK (GrB_Vector_new (&V, GrB_FP64, n)) ;
    ERR (GxB_Vector_unload (V, (void **) &X5, &type, &n5, &X5_size, &read_only, NULL)) ;
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
    OK (GxB_load_Vector_from_Container (V, Container, NULL)) ;
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

