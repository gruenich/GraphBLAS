//------------------------------------------------------------------------------
// GB_Matrix_new: create a new matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit (but disabled)

// The new matrix is nrows-by-ncols, with no entries in it.  Default format for
// an empty matrix is hypersparse CSC: A->p is size 2 and all zero, A->h is
// size 1, A->plen is 1, and contents A->x and A->i are NULL.  If this method
// fails, *A is set to NULL.

// FIXME: create global and per-matrix setting:
//
// For Ap and Ai, independently:
//
//  default: prefer 32 for global and per-matrix settings
//
//  Global settings: no matrix is converted if this is changed
//
//  strict_64   : report error if any matrix found to be 32
//  strict_32   : report error if any matrix found to be 64
//  prefer_32   : use 32 (or 64 if required) for new or recomputed matrices;
//                  any prior 64 ok (default)

//  per-matrix settings (only available if the global setting is prefer_64 or
//      prefer_32):
//
//  strict_64   : use 64, convert now if already 32 (not an error); lock and do
//                  not change in the future

//  strict_32   : use 32, convert now if already 64 (report error if too big
//                  for 32 and leave mtx and its per-matrix setting unchanged).
//                  lock the matrix and do not change in the future

//  prefer_32   : use 32, but do not convert now if already 64.
//                  Also stays as 64 bit if the matrix is too large for 32.
//                  May change to 32 in the future, at the discretion of the
//                  library, if the matrix appears as an output matrix (such as
//                  C for C=A*B), or by a call to GrB_wait.  No change is made
//                  if the matrix is used as an input, such as A for C=A*B.

// Changing the global settings has no impact on the block/non-blocking status
// of any existing matrix.  If the per-matrix setting is changed, it may cause
// future pending work that will be finalized by GrB_wait on that matrix.  If
// GrB_wait is called to materialize the matrix, and the matrix is not modified
// afterwards, it remains materialized and is not changed.

// If the global setting is strict_64 or strict_32, no per-matrix setting may
// be changed, even if the requested per-matrix setting matches the current
// global setting.  Existing matrices with a setting that doesn't match the
// current global strict_64 or strict_32 setting will cause an error if they
// are passed to any method except GrB_free.

#include "GB.h"

GrB_Info GB_Matrix_new          // create a new matrix with no entries
(
    GrB_Matrix *A,              // handle of matrix to create
    GrB_Type type,              // type of matrix to create
    GrB_Index nrows,            // matrix dimension is nrows-by-ncols
    GrB_Index ncols
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_CHECK_INIT ;
//  GB_WHERE0 ("GrB_Matrix_new (&A, type, nrows, ncols)") ; // FIXME will need
    GB_RETURN_IF_NULL (A) ;
    (*A) = NULL ;
    GB_RETURN_IF_NULL_OR_FAULTY (type) ;

    if (nrows > GB_NMAX || ncols > GB_NMAX)
    { 
        // problem too large
        return (GrB_INVALID_VALUE) ;
    }

    //--------------------------------------------------------------------------
    // create the matrix
    //--------------------------------------------------------------------------

    int64_t vlen, vdim ;
    bool A_is_csc ;
    if (ncols == 1)
    { 
        // n-by-1 matrices are always held by column, including 1-by-1
        A_is_csc = true ;
    }
    else if (nrows == 1)
    { 
        // 1-by-n matrices (except 1-by-1) are always held by row
        A_is_csc = false ;
    }
    else
    { 
        // m-by-n (including 0-by-0) with m != and n != use the global setting
        A_is_csc = GB_Global_is_csc_get ( ) ;
    }

    if (A_is_csc)
    { 
        vlen = (int64_t) nrows ;
        vdim = (int64_t) ncols ;
    }
    else
    { 
        vlen = (int64_t) ncols ;
        vdim = (int64_t) nrows ;
    }

    bool Ap_is_32 = false ; // GB_validate_p_is_32 (true, 1) ;  // FIXME
    bool Ai_is_32 = false ; // GB_validate_i_is_32 (true, avlen, avdim) ;

    return (GB_new (A, // auto sparsity, new header
        type, vlen, vdim, GB_ph_calloc, A_is_csc, GxB_AUTO_SPARSITY,
        GB_Global_hyper_switch_get ( ), 1, Ap_is_32, Ai_is_32)) ;
}

