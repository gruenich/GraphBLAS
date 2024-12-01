//------------------------------------------------------------------------------
// GB_debug_lookup: find k where j == Ah [k], without using the A->Y hyper_hash
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit

#ifndef GB_DEBUG_LOOKUP_H
#define GB_DEBUG_LOOKUP_H

#ifdef GB_DEBUG

// For a sparse, bitmap, or full matrix j == k.
// For a hypersparse matrix, find k so that j == Ah [k], if it
// appears in the list.

// k is not needed by the caller, just pstart, pend, pleft, and found.

// Once k is found, find pstart and pend, the start and end of the vector.
// pstart and pend are defined for all sparsity structures: hypersparse,
// sparse, bitmap, or full.

// With the introduction of the hyper_hash, this is used only for debugging.

static inline bool GB_debug_lookup  // find j = Ah [k]
(
    // input:
    const bool A_is_hyper,          // true if A is hypersparse
    const int64_t *restrict Ah,     // A->h [0..A->nvec-1]: list of vectors
    const int64_t *restrict Ap,     // A->p [0..A->nvec  ]: pointers to vectors
    const int64_t avlen,            // A->vlen
    // input/output:
    int64_t *restrict pleft,        // on input: look in A->h [pleft..pright].
                                    // on output: pleft == k if found.
    // input:
    int64_t pright,                 // normally A->nvec-1, but can be trimmed
    const int64_t j,                // vector to find, as j = Ah [k]
    // output:
    int64_t *restrict pstart,       // start of vector: Ap [k]
    int64_t *restrict pend          // end of vector: Ap [k+1]
)
{
    if (A_is_hyper)
    {
        // binary search of Ah [pleft...pright] for the value j
        bool found ;
        GB_BINARY_SEARCH (j, Ah, (*pleft), pright, found) ; // ok (historical)
        if (found)
        {
            // j appears in the hyperlist at Ah [pleft]
            // k = (*pleft)
            (*pstart) = Ap [(*pleft)] ;
            (*pend)   = Ap [(*pleft)+1] ;
        }
        else
        {
            // j does not appear in the hyperlist Ah
            // k = -1
            (*pstart) = -1 ;
            (*pend)   = -1 ;
        }
        return (found) ;
    }
    else
    {
        // A is sparse, bitmap, or full; j always appears
        // k = j
        (*pstart) = GBP (Ap, j, avlen) ;
        (*pend)   = GBP (Ap, j+1, avlen) ;
        return (true) ;
    }
}

#endif
#endif

