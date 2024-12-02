//------------------------------------------------------------------------------
// GB_hyper_hash_lookup: find k so that j == Ah [k], using the A->Y hyper_hash
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#ifndef GB_HYPER_HASH_LOOKUP_H
#define GB_HYPER_HASH_LOOKUP_H

#define GB_PTYPE uint32_t
#define GB_ITYPE uint32_t
#define GB_hyper_hash_lookup_T GB_hyper_hash_lookup_32_32
#include "include/GB_hyper_hash_lookup_template.h"

#define GB_PTYPE uint32_t
#define GB_ITYPE uint64_t
#define GB_hyper_hash_lookup_T GB_hyper_hash_lookup_32_64
#include "include/GB_hyper_hash_lookup_template.h"

#define GB_PTYPE uint64_t
#define GB_ITYPE uint32_t
#define GB_hyper_hash_lookup_T GB_hyper_hash_lookup_64_32
#include "include/GB_hyper_hash_lookup_template.h"

#define GB_PTYPE uint64_t
#define GB_ITYPE uint64_t
#define GB_hyper_hash_lookup_T GB_hyper_hash_lookup_64_64
#include "include/GB_hyper_hash_lookup_template.h"

GB_STATIC_INLINE int64_t GB_hyper_hash_lookup // k if j==Ah[k]; -1 if not found
(
    // inputs, not modified:
    const bool Ap_is_32,            // if true, Ap is 32-bit; else 64-bit
    const bool Ai_is_32,            // if true, Ah, Y->[pix] are 32-bit; else 64
    const void *Ah,                 // A->h [0..A->nvec-1]: list of vectors
    const int64_t anvec,
    const void *Ap,                 // A->p [0..A->nvec]: pointers to vectors
    const void *A_Yp,               // A->Y->p
    const void *A_Yi,               // A->Y->i
    const void *A_Yx,               // A->Y->x
    const uint64_t hash_bits,       // A->Y->vdim-1, which is hash table size-1
    const int64_t j,                // find j in Ah [0..anvec-1], using A->Y
    // outputs:
    int64_t *restrict pstart,       // start of vector: Ap [k]
    int64_t *restrict pend          // end of vector: Ap [k+1]
)
{
    if (Ap_is_32)
    {
        if (Ai_is_32)
        { 
            // Ap is 32-bit; Ah, A_Y[pix] are 32-bit
            GB_hyper_hash_lookup_32_32 (Ah, anvec, Ap, A_Yp, A_Yi, A_Yx,
                hash_bits, j, pstart, pend) ;
        }
        else
        { 
            // Ap is 32-bit; Ah, A_Y[pix] are 64-bit
            GB_hyper_hash_lookup_32_64 (Ah, anvec, Ap, A_Yp, A_Yi, A_Yx,
                hash_bits, j, pstart, pend) ;
        }
    }
    else
    {
        if (Ai_is_32)
        { 
            // Ap is 64-bit; Ah, A_Y[pix] are 32-bit
            GB_hyper_hash_lookup_64_32 (Ah, anvec, Ap, A_Yp, A_Yi, A_Yx,
                hash_bits, j, pstart, pend) ;
        }
        else
        { 
            // Ap is 64-bit; Ah, A_Y[pix] are 64-bit
            GB_hyper_hash_lookup_64_64 (Ah, anvec, Ap, A_Yp, A_Yi, A_Yx,
                hash_bits, j, pstart, pend) ;
        }
    }
}

#endif

