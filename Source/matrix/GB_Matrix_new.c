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

//      A->p        32-bit: UINT32_MAX entries, 64-bit: "inf"
//                  'column pointers', Ap [0] = 0, Ap [n] = nvals(A)
//                  size n+1, matrix is m-by-n
//
//                  [Ap [k] ... Ap [k+1]-1], kth nonempty col
//                  j = Ah [k], gives A(:,j)

//  let N = max(m,n)

//      A->i, A->h  : 32-bit: N < 2^31 (INT32_MAX), 64-bit: 2^60 (2^62)

// FIXME: create global and per-matrix setting:
//
// For Ap and Ai, independently:
//
//  Global settings: no matrix is converted if this is changed
//  ----------------
//
//  GxB_PREFER_32_BITS : use 32 (or 64 if required) for new or recomputed
//                      matrices; any prior 64 ok (will be the default; but use
//                      64-bit for now)
//
//  GxB_PREFER_64_BITS : use 64 by default (this is the default for now)
//
//  per-matrix settings:
//  -------------------
//
//  GxB_AUTO_BITS:  default : rely on the global settings

//  GxB_PREFER_32_BITS : use 32 bits if possible, but allow 64 bit if needed
//                      A hint.
//
//  GxB_PREFER_64_BITS : use 64 bits, convert now is already 32 bits,
//                      but 32 bit is not an error in the future.  A hint.
//
//  GxB_STRICT_32_BITS  : use 32 bits, but return an error if the matrix is too
//                      big for 32-bit.  convert now from 64 to 32.  If OK,
//                      lock the size of the integers and do not change in the
//                      future.  If the matrix nvals or dimension changes and
//                      32-bit becomes too small, return an error and do not
//                      convert the matrix.
//
//  GxB_STRICT_64_BITS : use 64, convert now if already 32;
//                      lock and do not change in the future.
//                      An error will occur if it 
//
// Changing the global settings has no impact on the block/non-blocking status
// of any existing matrix.  If the per-matrix setting is changed, it may cause
// future pending work that will be finalized by GrB_wait on that matrix.  If
// GrB_wait is called to materialize the matrix, and the matrix is not modified
// afterwards, it remains materialized and is not changed.

#define GB_FREE_ALL GB_Matrix_free (A)

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

    GrB_Info info ;
    GB_CHECK_INIT ;
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
        // m-by-n (including 0-by-0) with m != 1 or n != 1 use global setting
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

#if 0
    // get global pi_control
    int8_t p_control = GB_Global_p_control_get ( ) ;
    int8_t i_control = GB_Global_i_control_get ( ) ;
#else
    // HACK for now:
    int8_t p_control = GxB_PREFER_32_BITS ;
    int8_t i_control = GxB_PREFER_32_BITS ;
#endif

    // determine the p_is_32 and i_is_32 settings for the new matrix
    bool Ap_is_32, Ai_is_32 ;
    GB_OK (GB_determine_pi_is_32 (&Ap_is_32, &Ai_is_32, p_control, i_control,
        GxB_AUTO_SPARSITY, 1, vlen, vdim, true)) ;

    // create the matrix
    GB_OK (GB_new (A, // auto sparsity (sparse/hyper), new header
        type, vlen, vdim, GB_ph_calloc, A_is_csc, GxB_AUTO_SPARSITY,
        GB_Global_hyper_switch_get ( ), 1, Ap_is_32, Ai_is_32)) ;

    // HACK for now:
    ASSERT_MATRIX_OK (*A, "GrB_Matrix_new before convert", GB0) ;
    GB_OK (GB_convert_int (*A, false, false)) ; // FIXME
    ASSERT_MATRIX_OK (*A, "GrB_Matrix_new after convert", GB0) ;
    GB_OK (GB_valid_matrix (*A)) ; 

    return (GrB_SUCCESS) ;
}

