//------------------------------------------------------------------------------
// GB_bix_alloc: allocate a matrix to hold a given number of entries
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit, partially done

// Does not modify A->p or A->h.  Frees A->b, A->x,
// and A->i and reallocates them to the requested size.  Frees any pending
// tuples and deletes all entries (including zombies, if any).  If numeric is
// false, then A->x is freed but not reallocated.

// If this method fails, A->b, A->i, and A->x are NULL,
// but A->p and A->h are not modified.

// FIXME: revise A->p_is_32 if nzmax is too large, and realloc A->p.

#include "GB.h"

GB_CALLBACK_BIX_ALLOC_PROTO (GB_bix_alloc)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (A != NULL) ;

    //--------------------------------------------------------------------------
    // allocate the A->b, A->x, and A->i content of the matrix
    //--------------------------------------------------------------------------

    // Free the existing A->b, A->x, and A->i content, if any.
    // Leave A->p and A->h unchanged.
    GB_bix_free (A) ;
    A->iso = A_iso  ;       // OK: see caller for iso burble

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
        if (A->p_is_32 && nzmax >= UINT32_MAX)
        { 
            // FIXME: how to handle this case?
            ok = false ;
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
        A->x = GB_XALLOC (sparsity == GxB_BITMAP, A_iso,    // x:OK
            nzmax, A->type->size, &(A->x_size)) ;
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

