//------------------------------------------------------------------------------
// gbextractvalues: extract all entries from a GraphBLAS matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Usage:

// X = gbextractvalues (A)

#include "gb_interface.h"

#define USAGE "usage: X = GrB.extractvalues (A)"

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    gb_usage (nargin == 1 && nargout <= 1, USAGE) ;

    //--------------------------------------------------------------------------
    // get the matrix
    //--------------------------------------------------------------------------

    GrB_Matrix A = gb_get_shallow (pargin [0]) ;
    uint64_t nrows, ncols, nvals ;
    OK (GrB_Matrix_nrows (&nrows, A)) ;
    OK (GrB_Matrix_ncols (&ncols, A)) ;
    int burble ;
    bool disable_burble = (nrows <= 1 && ncols <= 1) ;
    if (disable_burble)
    {
        OK (GrB_Global_get_INT32 (GrB_GLOBAL, &burble, GxB_BURBLE)) ;
        OK (GrB_Global_set_INT32 (GrB_GLOBAL, false, GxB_BURBLE)) ;
    }

//  OK (GrB_Matrix_nvals (&nvals, A)) ;
//  OK (GxB_Matrix_type (&xtype, A)) ;
//  uint64_t s = MAX (nvals, 1) ;

    //--------------------------------------------------------------------------
    // extract the tuples
    //--------------------------------------------------------------------------

    void *X = NULL ;
    GrB_Type xtype = NULL ;
    bool read_only = false ;
    size_t X_size = 0 ;
    GrB_Vector X_vector = NULL ;
    OK (GrB_Vector_new (&X_vector, GrB_FP64, 0)) ;
    OK (GxB_Matrix_extractTuples_Vector (NULL, NULL, X_vector, A, NULL)) ;
    OK (GxB_Vector_unload (X_vector, &X, &nvals, &X_size, &xtype, &read_only,
        NULL)) ;
    pargout [0] = gb_export_to_mxfull ((void **) &X, nvals, 1, xtype) ;

    //--------------------------------------------------------------------------
    // free workspace
    //--------------------------------------------------------------------------

    OK (GrB_Matrix_free (&A)) ;
    OK (GrB_Vector_free (&X_vector)) ;
    if (disable_burble)
    { 
        OK (GrB_Global_set_INT32 (GrB_GLOBAL, burble, GxB_BURBLE)) ;
    }
    GB_WRAPUP ;
}

