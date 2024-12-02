//------------------------------------------------------------------------------
// GB_debug_lookup_template: find k where j == Ah [k], no hyper_hash
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// For debugging only.

static inline bool GB_debug_lookup_T // find j = Ah [k]
(
    // input:
    const bool A_is_hyper,          // true if A is hypersparse
    const GB_ITYPE *restrict Ah,    // A->h [0..A->nvec-1]: list of vectors
    const GB_PTYPE *restrict Ap,    // A->p [0..A->nvec  ]: pointers to vectors
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
        GB_BINARY_SEARCH (j, Ah, (*pleft), pright, found) ;
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

#undef GB_PTYPE
#undef GB_ITYPE
#undef GB_debug_lookup_T

