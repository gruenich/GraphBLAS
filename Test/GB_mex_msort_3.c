//------------------------------------------------------------------------------
// GB_mex_msort_3: sort using GB_msort_3
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_mex.h"

#define USAGE "[I,J,K] = GB_mex_msort_3 (I,J,K,nthreads)"

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{
    bool malloc_debug = GB_mx_get_global (true) ;

    // check inputs
    if (nargin != 4 || nargout != 3)
    {
        mexErrMsgTxt ("Usage: " USAGE) ;
    }
    if (!mxIsClass (pargin [0], "uint64"))
    {
        mexErrMsgTxt ("I must be a uint64 array") ;
    }
    if (!mxIsClass (pargin [1], "uint64"))
    {
        mexErrMsgTxt ("J must be a uint64 array") ;
    }
    if (!mxIsClass (pargin [2], "uint64"))
    {
        mexErrMsgTxt ("K must be a uint64 array") ;
    }

    uint64_t *I = mxGetData (pargin [0]) ;
    int64_t n = (uint64_t) mxGetNumberOfElements (pargin [0]) ;

    uint64_t *J = mxGetData (pargin [1]) ;
    if (n != (uint64_t) mxGetNumberOfElements (pargin [1])) 
    {
        mexErrMsgTxt ("I and J must be the same length") ;
    }

    uint64_t *K = mxGetData (pargin [2]) ;
    if (n != (uint64_t) mxGetNumberOfElements (pargin [2])) 
    {
        mexErrMsgTxt ("I and K must be the same length") ;
    }

    int GET_SCALAR (3, int, nthreads, 1) ;

    pargout [0] = GB_mx_create_full (n, 1, GrB_INT64) ;
    uint64_t *Iout = mxGetData (pargout [0]) ;
    memcpy (Iout, I, n * sizeof (uint64_t)) ;

    pargout [1] = GB_mx_create_full (n, 1, GrB_INT64) ;
    uint64_t *Jout = mxGetData (pargout [1]) ;
    memcpy (Jout, J, n * sizeof (uint64_t)) ;

    pargout [2] = GB_mx_create_full (n, 1, GrB_INT64) ;
    uint64_t *Kout = mxGetData (pargout [2]) ;
    memcpy (Kout, K, n * sizeof (uint64_t)) ;

    GB_msort_3 (Iout, Jout, Kout, n, nthreads) ;

    GB_mx_put_global (true) ;   
}

