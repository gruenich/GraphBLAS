//------------------------------------------------------------------------------
// GB_Pending.h: operations for pending tuples
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#ifndef GB_PENDING_H
#define GB_PENDING_H
#include "GB.h"

//------------------------------------------------------------------------------
// GB_Pending functions
//------------------------------------------------------------------------------

bool GB_Pending_alloc       // create a list of pending tuples
(
    GB_Pending *PHandle,    // output
    bool iso,               // if true, do not allocate Pending->x
    GrB_Type type,          // type of pending tuples
    GrB_BinaryOp op,        // operator for assembling pending tuples
    bool is_matrix,         // true if Pending->j must be allocated
    int64_t nmax            // # of pending tuples to hold
) ;

bool GB_Pending_realloc     // reallocate a list of pending tuples
(
    GB_Pending *PHandle,    // Pending tuple list to reallocate
    int64_t nnew,           // # of new tuples to accomodate
    GB_Werk Werk
) ;

void GB_Pending_free        // free a list of pending tuples
(
    GB_Pending *PHandle
) ;

//------------------------------------------------------------------------------
// GB_Pending_add:  add an entry C(i,j) to the list of pending tuples
//------------------------------------------------------------------------------

static inline bool GB_Pending_add   // add a tuple to the list
(
    GrB_Matrix C,           // matrix to add a pending tuple to
    const GB_void *scalar,  // scalar to add to the pending tuples
    const GrB_Type type,    // scalar type, if list is created
    const GrB_BinaryOp op,  // new Pending->op, if list is created
    const int64_t i,        // index into vector
    const int64_t j,        // vector index
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (C != NULL) ;
    ASSERT (C->Pending != NULL) ;

    //--------------------------------------------------------------------------
    // allocate the Pending tuples, or ensure existing list is large enough
    //--------------------------------------------------------------------------

    bool iso = C->iso ;
    bool is_matrix = (C->vdim > 1) ;
    if (!GB_Pending_ensure (&(C->Pending), iso, type, op, is_matrix, 1, Werk))
    {
        return (false) ;
    }
    GB_Pending Pending = C->Pending ;
    int64_t n = Pending->n ;

    ASSERT (Pending->type == type) ;
    ASSERT (Pending->nmax > 0 && n < Pending->nmax) ;
    ASSERT (Pending->i != NULL) ;
    ASSERT ((is_matrix) == (Pending->j != NULL)) ;

    //--------------------------------------------------------------------------
    // keep track of whether or not the pending tuples are already sorted
    //--------------------------------------------------------------------------

    GB_CPending_DECLARE (Pending_i) ; GB_CPending_PTR (Pending_i, C, i) ;
    GB_CPending_DECLARE (Pending_j) ; GB_CPending_PTR (Pending_j, C, j) ;

    if (n > 0 && Pending->sorted)
    { 
        int64_t ilast = GB_IGET (Pending_i, n-1) ;
        int64_t jlast = (Pending_j != NULL) ? GB_IGET (Pending_j, n-1) : 0 ;
        Pending->sorted = (jlast < j) || (jlast == j && ilast <= i) ;
    }

    //--------------------------------------------------------------------------
    // add the (i,j,scalar) or just (i,scalar) if Pending->j is NULL
    //--------------------------------------------------------------------------

    // Pending_i [n] = i ;
    GB_ISET (Pending_i, n, i) ;
    if (Pending_j != NULL)
    { 
        // Pending_j [n] = j ;
        GB_ISET (Pending_j, n, j) ;
    }
    size_t size = type->size ;
    GB_void *restrict Pending_x = Pending->x ;
    if (Pending_x != NULL)
    {
        memcpy (Pending_x +(n*size), scalar, size) ;
    }
    Pending->n++ ;

    return (true) ;     // success
}

#endif

