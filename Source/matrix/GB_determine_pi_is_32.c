//------------------------------------------------------------------------------
// GB_determine_pi_is_32: determine p_is_32 and i_is_32 for a new matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#include "GB.h"

// The caller has determined the pi_control for new matrices it will create,
// typically with:
//
//      p_control = GB_pi_control (C->p_control, GB_Global_p_control_get ( )) ;
//      i_control = GB_pi_control (C->i_control, GB_Global_i_control_get ( )) ;
//
//  or, if it has no output matrix C, simply use:
//
//      p_control = GB_Global_p_control_get ( ) ;
//      i_control = GB_Global_p_control_get ( ) ;
//
// If the global or per-matrix controls are not relevant, simply use the
// following to use the smallest valid integer sizes:
//
//      p_control = GxB_PREFER_32 ;
//      i_control = GxB_PREFER_32 ;
//
// This method then determines the final p_is_32 and i_is_32 for a new matrix
// of the requested size.
//
// If this is a temporary matrix and not the final the output matrix C, then
// strict can be passed as false.  In this case, the strict control condition
// is not checked.  The resulting temporary matrix may then use different
// integer sizes than the final C matrix.  The caller must ensure that if the
// resulting matrix is transplanted into C, that GB_convert_int must be used
// to then conform C to any strict pi_control.

GrB_Info GB_determine_pi_is_32 // determine p_is_32 and i_is_32 for a new matrix
(
    // output
    bool *p_is_32,      // if true, Ap will be 32 bits; else 64
    bool *i_is_32,      // if true, Ai etc will be 32 bits; else 64
    // input
    int8_t p_control,   // effective p_control for the caller
    int8_t i_control,   // effective i_control for the caller
    int sparsity,       // sparse, hyper, bitmap, full, or auto (sparse/hyper)
    int64_t nvals,      // lower bound on # of entries in the matrix to create
    int64_t vlen,       // dimensions of the matrix to create
    int64_t vdim,
    bool strict         // if true, check strict controls.  Otherwise, this
                        // matrix is a temporary one that will not be
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (p_is_32 != NULL) ;
    ASSERT (i_is_32 != NULL) ;

    //--------------------------------------------------------------------------
    // determine the 32/64 bit integer sizes for a new matrix
    //--------------------------------------------------------------------------

    if (sparsity == GxB_FULL || sparsity == GxB_BITMAP)
    {
    
        //----------------------------------------------------------------------
        // full/bitmap matrices do not have any integer sizes
        //----------------------------------------------------------------------

        (*p_is_32) = false ;
        (*i_is_32) = false ;

    }
    else
    {

        //----------------------------------------------------------------------
        // determine the 32/64 integer sizes for a sparse/hypersparse matrix
        //----------------------------------------------------------------------

        // determine ideal 32/64 sizes for any matrix created by the caller
        bool p_prefer_32 = (p_control <= GxB_PREFER_32_BITS) ;
        bool i_prefer_32 = (i_control <= GxB_PREFER_32_BITS) ;

        // revise them accordering to the matrix content
        (*p_is_32) = GB_validate_p_is_32 (p_prefer_32, nvals) ;
        (*i_is_32) = GB_validate_i_is_32 (i_prefer_32, vlen, vdim) ;

        // check the matrix for strict pi_controls
        if (strict)
        { 
            if (!GB_valid_strict (p_control, *p_is_32) ||
                !GB_valid_strict (i_control, *i_is_32))
            {
                // control is strict, but the matrix won't fit
                return (GrB_INVALID_VALUE) ;
            }
        }
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    return (GrB_SUCCESS) ;
}

