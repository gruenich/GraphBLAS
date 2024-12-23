//------------------------------------------------------------------------------
// GB_hyper_realloc: reallocate a matrix hyperlist
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Change the size of the A->h and A->p hyperlist.
// No change is made if A is not hypersparse.
// No change is made to A->Y.
// A->p_is_32 and A->i_is_32 are unchanged.

#include "GB.h"

GrB_Info GB_hyper_realloc
(
    GrB_Matrix A,               // matrix with hyperlist to reallocate
    int64_t plen_new,           // new size of A->p and A->h
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (A != NULL) ;
    ASSERT (GB_ZOMBIES_OK (A)) ;        // pattern not accessed
    ASSERT (GB_JUMBLED_OK (A)) ;
    ASSERT (GB_PENDING_OK (A)) ;

    //--------------------------------------------------------------------------
    // reallocate the hyperlist
    //--------------------------------------------------------------------------

    if (GB_IS_HYPERSPARSE (A))
    {
        ASSERT (!A->p_shallow) ;
        ASSERT (!A->h_shallow) ;

        #ifdef GB_DEBUG
        int64_t plen_old = A->plen ;            // old size of A->p and A->h
        #endif
        plen_new = GB_IMAX (1, plen_new) ;      // new size of A->p and A->h
        size_t psize = (A->p_is_32) ? sizeof (uint32_t) : sizeof (uint64_t) ;
        size_t isize = (A->i_is_32) ? sizeof (uint32_t) : sizeof (uint64_t) ;

        // change the size of A->h and A->p
        bool ok1 = true, ok2 = true ;
        A->p = GB_realloc_memory (plen_new+1, psize, A->p, &(A->p_size), &ok1) ;
        A->h = GB_realloc_memory (plen_new  , isize, A->h, &(A->h_size), &ok2) ;
        bool ok = ok1 && ok2 ;

        // always succeeds if the space shrinks
        ASSERT (GB_IMPLIES (plen_new <= plen_old, ok)) ;

        if (!ok)
        { 
            // out of memory
            GB_phybix_free (A) ;
            return (GrB_OUT_OF_MEMORY) ;
        }

        // size of A->p and A->h has been changed
        A->plen = plen_new ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    return (GrB_SUCCESS) ;
}

