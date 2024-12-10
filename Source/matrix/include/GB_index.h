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
// GB_valid_[pi]_is_32: returns true if [pi] settings are OK
//------------------------------------------------------------------------------

// returns true if the pi settings are OK for this matrix.
// returns false if the matrix is too large for its pi settings

static inline bool GB_valid_p_is_32
(
    bool p_is_32,   // if true, A->p is 32-bit; else 64-bit
    int64_t nvals   // # of entries in the matrix
)
{
    // matrix is valid if A->p is 64 bit, or nvals is small enough
    return (!p_is_32 || nvals < UINT32_MAX) ;
}

static inline bool GB_valid_i_is_32
(
    bool i_is_32,   // if true, A->[hi] and A->Y are 32-bit; else 64-bit
    int64_t vlen,   // matrix dimentions
    int64_t vdim
)
{
    // matrix is valid if A->i is 64 bit, or the dimensions are small enough
    return (!i_is_32 || GB_IMAX (vlen, vdim) <= GB_NMAX32) ;
}

static inline bool GB_valid_pi_is_32
(
    bool p_is_32,   // if true, A->p is 32-bit; else 64-bit
    bool i_is_32,   // if true, A->[hi] and A->Y are 32-bit; else 64-bit
    int64_t nvals,  // # of entries in the matrix
    int64_t vlen,   // matrix dimentions
    int64_t vdim
)
{
    // matrix is valid if A->p is 64 bit, or nvals is small enough, and
    // if A->i is 64 bit, or the dimensions are small enough.
    return (GB_valid_p_is_32 (p_is_32, nvals) &&
            GB_valid_i_is_32 (i_is_32, vlen, vdim)) ;
}

//------------------------------------------------------------------------------
// GB_valid_strict: return true if a strict integer control setting is valid
//------------------------------------------------------------------------------

static inline bool GB_valid_strict
(
    int8_t pi_control,  // per-matrix p_control or i_control
    bool pi_is_32       // p_is_32 or i_is_32 for a given matrix
)
{
    // if the setting for the matrix is strict, make sure it has that status
    if (pi_control == GxB_STRICT_64_BITS)
    {
        // the matrix must be 64-bit
        return (!pi_is_32) ;
    }
    else if (pi_control == GxB_STRICT_32_BITS)
    {
        // the matrix must be 32-bit
        return (pi_is_32) ;
    }
    else
    {
        // all other settings are fine
        return (true) ;
    }
}

//------------------------------------------------------------------------------
// GB_pi_control: return effective p_control or i_control
//------------------------------------------------------------------------------

static inline int8_t GB_pi_control
(
    int8_t matrix_pi_control,
    int8_t global_pi_control
)
{
    if (matrix_pi_control == GxB_AUTO_BITS)
    {
        // default: matrix control defers to the global control
        return (global_pi_control) ;
    }
    else
    {
        // use the matrix-specific control
        return (matrix_pi_control) ;
    }
}

//------------------------------------------------------------------------------
// GB_valid_matrix: check if a matrix is valid, based on the current control
//------------------------------------------------------------------------------

static inline GrB_Info GB_valid_matrix // returns GrB_SUCCESS, or error
(
    GrB_Matrix A                // matrix to validate
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

    // check the matrix for strict controls
    if (!GB_valid_strict (A->p_control, A->p_is_32) ||
        !GB_valid_strict (A->i_control, A->i_is_32))
    { 
        return (GrB_INVALID_OBJECT) ;
    }

    // assert that the matrix status is large enough for its content
    #ifdef GB_DEBUG
    ASSERT (GB_valid_p_is_32 (A->p_is_32, A->nvals)) ;
    ASSERT (GB_valid_i_is_32 (A->i_is_32, A->vlen, A->vdim)) ;
    #endif

    // HACK for now: assume all inputs/outputs to GrB* methods are 64-bit
    GB_assert (!A->p_is_32) ;
    GB_assert (!A->i_is_32) ;

    return (GrB_SUCCESS) ;
}

#endif

