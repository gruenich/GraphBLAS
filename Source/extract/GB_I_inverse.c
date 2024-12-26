//------------------------------------------------------------------------------
// GB_I_inverse: invert an index list
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// I is a large list relative to the vector length, avlen, and it is not
// contiguous.  Scatter I into the I inverse buckets (Ihead and Inext) for quick
// lookup.

// FUTURE:: this code is sequential.  Constructing the I inverse buckets in
// parallel would require synchronization (a critical section for each bucket,
// or atomics).  A more parallel approach might use qsort first, to find
// duplicates in I, and then construct the buckets in parallel after the qsort.
// But the time complexity would be higher.

#define GB_DEBUG

#include "extract/GB_subref.h"

GrB_Info GB_I_inverse           // invert the I list for C=A(I,:)
(
    const void *I,              // list of indices, duplicates OK
    const bool I_is_32,         // if true, I is 32-bit; else 64 bit
    int64_t nI,                 // length of I
    int64_t avlen,              // length of the vectors of A
    // outputs:
    uint64_t *restrict *p_Ihead,    // head pointers for buckets, size avlen
    size_t *p_Ihead_size,
    uint64_t *restrict *p_Inext,    // next pointers for buckets, size nI
    size_t *p_Inext_size,
    int64_t *p_nduplicates,     // number of duplicate entries in I
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    uint64_t *Ihead  = NULL ; size_t Ihead_size = 0 ;
    uint64_t *Inext = NULL ; size_t Inext_size = 0 ;
    int64_t nduplicates = 0 ;

    (*p_Ihead) = NULL ; (*p_Ihead_size) = 0 ;
    (*p_Inext) = NULL ; (*p_Inext_size) = 0 ;
    (*p_nduplicates) = 0 ;

    GB_IDECL (I, const, u) ; GB_IPTR (I, I_is_32) ;

    //--------------------------------------------------------------------------
    // allocate workspace
    //--------------------------------------------------------------------------

    Ihead = GB_MALLOC_WORK (avlen, uint64_t, &Ihead_size) ;
    Inext = GB_MALLOC_WORK (nI,    uint64_t, &Inext_size) ;
    if (Inext == NULL || Ihead == NULL)
    { 
        // out of memory
        GB_FREE_WORK (&Ihead, Ihead_size) ;
        GB_FREE_WORK (&Inext, Inext_size) ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    int nthreads_max = GB_Context_nthreads_max ( ) ;
    GB_memset (Ihead, 0xFF, Ihead_size, nthreads_max) ;

    //--------------------------------------------------------------------------
    // scatter the I indices into buckets
    //--------------------------------------------------------------------------

    // At this point, Ihead [0..avlen-1] = UINT64_MAX.

    // O(nI) time; not parallel
    for (int64_t inew = nI-1 ; inew >= 0 ; inew--)
    {
        int64_t i = GB_IGET (I, inew) ;
        ASSERT (i >= 0 && i < avlen) ;
        int64_t ihead = Ihead [i] ;
        if (ihead > nI)
        { 
            // first time i has been seen in the list I
            ASSERT (ihead == UINT64_MAX) ;
        }
        else
        { 
            // i has already been seen in the list I
            nduplicates++ ;
        }
        Ihead [i] = inew ;
        Inext [inew] = ihead ;
    }

    // indices in I are now in buckets.  An index i might appear more than once
    // in the list I.  inew = Ihead [i] is the first position of i in I (i will
    // be I [inew]), Ihead [i] is the head of a link list of all places where i
    // appears in I.  inew = Inext [inew] traverses this list, until inew is >=
    // nI, which denotes the end of the bucket.

    // to traverse all entries in bucket i, do:
    // GB_for_each_index_in_bucket (inew,i,nI,Ihead,Inext) { ... }

    #define GB_for_each_index_in_bucket(inew,i,nI,Ihead,Inext) \
        for (uint64_t inew = Ihead [i] ; inew < nI ; inew = Inext [inew])

    // If Ihead [i] > nI, then the ith bucket is empty and i is not in I.
    // Otherise, the first index in bucket i is Ihead [i].

    #ifdef GB_DEBUG
    for (int64_t i = 0 ; i < avlen ; i++)
    {
        GB_for_each_index_in_bucket (inew, i, nI, Ihead, Inext)
        {
            // inew is the new index in C, and i is the index in A.
            // All entries in the ith bucket refer to the same row A(i,:),
            // but with different indices C (inew,:) in C.
            ASSERT (inew >= 0 && inew < nI) ;
            ASSERT (i == GB_IGET (I, inew)) ;
        }
    }
    #endif

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    (*p_Ihead) = Ihead ; (*p_Ihead_size) = Ihead_size ;
    (*p_Inext) = Inext ; (*p_Inext_size) = Inext_size ;
    (*p_nduplicates) = nduplicates ;
    return (GrB_SUCCESS) ;
}

