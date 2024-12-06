//------------------------------------------------------------------------------
// GB_index.h: definitions for index lists and types of assignments
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#ifndef GB_INDEX_H
#define GB_INDEX_H

//------------------------------------------------------------------------------
// maximum matrix or vector dimension
//------------------------------------------------------------------------------

#ifndef GrB_INDEX_MAX
#define GrB_INDEX_MAX ((uint64_t) (1ULL << 60) - 1)
#endif

#ifndef GxB_INDEX32_MAX
#define GxB_INDEX32_MAX ((uint64_t) (1ULL << 30) - 1)
#endif

// GB_NMAX:   max dimension when A->i is 64-bit
// GB_NMAX32: max dimension when A->i is 32-bit
#define GB_NMAX   ((uint64_t) (1ULL << 60))
#define GB_NMAX32 ((uint64_t) (1ULL << 30))

static inline bool GB_validate_i_is_32
(
    bool i_is_32,       // if true, requesting 32-bit indices
    int64_t vlen,       // vector length of the matrix
    int64_t vdim        // vector dimension of the matrix
)
{
    if (i_is_32 && GB_IMAX (vlen, vdim) > GB_NMAX32)
    { 
        // A->i is requested too small; make it 64-bit
        i_is_32 = false ;
    }
    return (i_is_32) ;
}

static inline bool GB_validate_p_is_32
(
    bool p_is_32,       // if true, requesting 32-bit offsets
    int64_t nvals_max   // max # of entries in the matrix
)
{
    if (p_is_32 && nvals_max >= UINT32_MAX)
    { 
        // A->p is requested too small; make it 64-bit
        p_is_32 = false ;
    }
    return (p_is_32) ;
}

#endif

