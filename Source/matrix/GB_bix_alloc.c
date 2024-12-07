//------------------------------------------------------------------------------
// GB_bix_alloc: allocate a matrix to hold a given number of entries
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Does not modify A->p or A->h.  Frees A->b, A->x, and A->i and reallocates
// them to the requested size.  Frees any pending tuples and deletes all
// entries (including zombies, if any).  If numeric is false, then A->x is
// freed but not reallocated.

// If A->p_is_32 or A->i_is_32 are invalid, GrB_[FIXME] is returned and the
// allocation fails.  If this method fails, A->b, A->i, and A->x are NULL
// (having been freed if already present), but A->p and A->h are not modified.

#include "GB.h"

// The prototype is in Source/callback:
//
//  GrB_Info GB_bix_alloc       // allocate A->b, A->i, and A->x in a matrix
//  (
//      GrB_Matrix A,           // matrix to allocate space for
//      const GrB_Index nzmax,  // number of entries the matrix can hold
//                              // ignored if A is iso and full
//      const int sparsity,     // sparse (=hyper/auto) / bitmap / full
//      const bool bitmap_calloc,   // if true, calloc A->b, else use malloc
//      const bool numeric,     // if true, allocate A->x, else A->x is NULL
//      const bool A_iso        // if true, allocate A as iso
//  )

GB_CALLBACK_BIX_ALLOC_PROTO (GB_bix_alloc)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (A != NULL) ;
    ASSERT (GB_IMPLIES (sparsity == GxB_FULL || sparsity == GxB_BITMAP,
        !(A->p_is_32) && !(A->i_is_32))) ;

    //--------------------------------------------------------------------------
    // allocate the A->b, A->x, and A->i content of the matrix
    //--------------------------------------------------------------------------

    // Free the existing A->b, A->x, and A->i content, if any.
    // Leave A->p and A->h unchanged.
    GB_bix_free (A) ;
    A->iso = A_iso ;

    bool ok = true ;
    if (sparsity == GxB_BITMAP)
    {
        if (bitmap_calloc)
        { 
            // content is fully defined
            A->b = GB_CALLOC (nzmax, int8_t, &(A->b_size)) ;
            A->magic = GB_MAGIC ;
        }
        else
        { 
            // bitmap is not defined and will be computed by the caller
            A->b = GB_MALLOC (nzmax, int8_t, &(A->b_size)) ;
        }
        ok = (A->b != NULL) ;
    }
    else if (sparsity != GxB_FULL)
    {
        // sparsity: sparse or hypersparse
        if ((A->p_is_32 != GB_validate_p_is_32 (A->p_is_32, nzmax)) ||
            (A->i_is_32 != GB_validate_i_is_32 (A->i_is_32, A->vlen, A->vdim)))
        { 
            // p_is_32 and/or i_is_32 are invalid; cannot allocate safely
            return (-911) ;     // FIXME: p_is_32 or i_is_32 invalid
        }
        size_t isize = A->i_is_32 ? sizeof (int32_t) : sizeof (int64_t) ;
        A->i = GB_malloc_memory (nzmax, isize, &(A->i_size)) ;
        ok = (A->i != NULL) ;
        if (ok)
        { 
            // Ai [0] = 0
            memset (A->i, 0, isize) ;
        }
    }

    if (numeric)
    { 
        // calloc the space if A is bitmap
        A->x = GB_XALLOC (sparsity == GxB_BITMAP, A_iso, nzmax, A->type->size,
            &(A->x_size)) ;
        ok = ok && (A->x != NULL) ;
    }

    if (!ok)
    { 
        // out of memory
        GB_bix_free (A) ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    return (GrB_SUCCESS) ;
}

