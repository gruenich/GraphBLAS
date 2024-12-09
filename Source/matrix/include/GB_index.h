//------------------------------------------------------------------------------
// GB_index.h: definitions for matrix indices and integers
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

//------------------------------------------------------------------------------
// GB_validate_i_is_32: returns a revised i_is_32, based on max dimension
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// GB_validate_p_is_32: returns a revised p_is_32, based on nvals_max
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// GB_valid_control: return true if an integer control setting is valid
//------------------------------------------------------------------------------

static inline bool GB_valid_control
(
    int8_t pi_control,  // global or per-matrix p_control or i_control
    bool pi_is_32       // p_is_32 or i_is_32 for a given matrix
)
{
    return (pi_control == GxB_AUTO_BITS ||
           (pi_control == GxB_STRICT_32_BITS &&  pi_is_32) ||
           (pi_control == GxB_STRICT_64_BITS && !pi_is_32)) ;
}

//------------------------------------------------------------------------------
// GB_valid: check if a matrix is valid, based on the current control
//------------------------------------------------------------------------------

static inline bool GB_valid     // returns GrB_SUCCESS, or error
(
    GrB_Matrix A,               // matrix to validate
    int8_t global_p_control,    // global p_control, from Global or Werk
    int8_t global_i_control     // global i_control, from Global or Werk
)
{

    // a NULL matrix is always valid so far (it may be an optional argument)
    if (A == NULL)
    { 
        return (GrB_SUCCESS) ;
    }

    // check the magic status
    if (A->magic != GB_MAGIC)
    { 
        if (A->magic == GB_MAGIC2)
        {
            return (GrB_INVALID_OBJECT) ;
        }
        else
        {
            return (GrB_UNINITIALIZED_OBJECT) ;
        }
    }

    // a full or bitmap matrix has no integers
    if (A->p == NULL && A->h == NULL && A->i == NULL && A->Y == NULL)
    {
        return (GrB_SUCCESS) ;
    }

    // check the global pi_controls
    if (!GB_valid_control (global_p_control, A->p_is_32))
    { 
        return (GrB_INVALID_OBJECT) ;
    }
    if (!GB_valid_control (global_i_control, A->i_is_32))
    { 
        return (GrB_INVALID_OBJECT) ;
    }

    // check the matrix pi_controls
    if (!GB_valid_control (A->p_control, A->p_is_32))
    { 
        return (GrB_INVALID_OBJECT) ;
    }
    if (!GB_valid_control (A->i_control, A->i_is_32))
    { 
        return (GrB_INVALID_OBJECT) ;
    }

    // assert that the matrix status is large enough for its content
    #ifdef GB_DEBUG
    ASSERT (!A->p_is_32 || GB_validate_p_is_32 (true, A->nvals)) ;
    ASSERT (!A->i_is_32 || GB_validate_i_is_32 (true, A->vlen, A->vdim)) ;
    #endif

    return (GrB_SUCCESS) ;
}

#endif

