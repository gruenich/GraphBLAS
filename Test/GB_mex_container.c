//------------------------------------------------------------------------------
// GB_mex_container: copy a matrix, by loading/unloading it into a container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// copy a matrix/vector via load/unload of a container

#include "GB_mex.h"
#include "GB_mex_errors.h"

#define USAGE "C = GB_mex_container (A)"

#define FREE_ALL                        \
{                                       \
    GrB_Matrix_free_(&C) ;              \
    GrB_Matrix_free_(&A) ;              \
    GxB_Container_free (&Container) ;   \
    GB_mx_put_global (true) ;           \
}

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    GrB_Info info ;
    bool malloc_debug = GB_mx_get_global (true) ;
    GxB_Container Container = NULL ;
    GrB_Matrix C = NULL, A = NULL ;

    // check inputs
    if (nargout > 1 || nargin != 1)
    {
        mexErrMsgTxt ("Usage: " USAGE) ;
    }

    #define GET_DEEP_COPY           \
        GrB_Matrix_dup (&C, A) ;    \
        GrB_Matrix_wait (C, GrB_MATERIALIZE) ;
    #define FREE_DEEP_COPY  GrB_Matrix_free (&C) ;

    // get a shallow copy of the input
    A = GB_mx_mxArray_to_Matrix (pargin [0], "A input", false, true) ;

    // C = A
    GET_DEEP_COPY ;

    METHOD (GxB_Container_new (&Container)) ;
    METHOD (GxB_unload_Matrix_into_Container (C, Container, NULL)) ;
    METHOD (GxB_load_Matrix_from_Container (C, Container, NULL)) ;
    OK (GxB_Container_free (&Container)) ;

    // return C as a struct and free the GraphBLAS C
    pargout [0] = GB_mx_Matrix_to_mxArray (&C, "C output", true) ;

    FREE_ALL ;
}

