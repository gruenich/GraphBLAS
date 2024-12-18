//------------------------------------------------------------------------------
// GB_p_slice: partition Ap for a set of tasks
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Ap [0..n] is an array with monotonically increasing entries.  This function
// slices Ap so that each chunk has the same number of total values of its
// entries.  Ap can be A->p for a matrix and then n = A->nvec.  Or it can be
// the work needed for computing n tasks, where Ap [p] is the work for task p.

// If Ap is NULL then the matrix A (not provided here) is full or bitmap,
// which this function handles (Ap is implicit).

#include "GB.h"

//------------------------------------------------------------------------------
// GB_p_slice_32 and GB_p_slice_64: for Ap as uint32_t or uint64_t
//------------------------------------------------------------------------------

#define GB_Ap_TYPE             uint32_t
#define GB_p_slice_TYPE        GB_p_slice_32
#define GB_p_slice_worker_TYPE GB_p_slice_worker_32
#define GB_trim_binary_search_TYPE GB_trim_binary_search_32
#include "slice/factory/GB_p_slice_template.c"

#define GB_Ap_TYPE             uint64_t
#define GB_p_slice_TYPE        GB_p_slice_64
#define GB_p_slice_worker_TYPE GB_p_slice_worker_64
#define GB_trim_binary_search_TYPE GB_trim_binary_search_64
#include "slice/factory/GB_p_slice_template.c"

//------------------------------------------------------------------------------
// GB_p_slice: partition Ap for a set of tasks
//------------------------------------------------------------------------------

//  void GB_p_slice                 // slice Ap, 32-bit or 64-bit
//  (
//      // output:
//      int64_t *restrict Slice,    // size ntasks+1
//      // input:
//      const void *Ap,             // array size n+1 (full/bitmap: NULL)
//      bool Ap_is_32,              // if true, Ap is uint32_t, else uint64_t
//      const int64_t n,
//      const int ntasks,           // # of tasks
//      const bool perfectly_balanced
//  )

GB_CALLBACK_P_SLICE_PROTO (GB_p_slice)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (Slice != NULL) ;
    #ifdef GB_DEBUG
    for (int taskid = 0 ; taskid <= ntasks ; taskid++)
    {
        Slice [taskid] = -1 ;
    }
    #endif

    //--------------------------------------------------------------------------
    // slice the work
    //--------------------------------------------------------------------------

    if (Ap == NULL)
    { 
        // A is full or bitmap: slice 0:n equally for all tasks
        GB_e_slice (Slice, n, ntasks) ;
    }
    else
    {
        // A is sparse or hypersparse
        if (Ap_is_32)
        { 
            // Ap is uint32_t
            GB_p_slice_32 (Slice, Ap, n, ntasks, perfectly_balanced) ;
        }
        else
        { 
            // Ap is uint64_t
            GB_p_slice_64 (Slice, Ap, n, ntasks, perfectly_balanced) ;
        }
    }

    //--------------------------------------------------------------------------
    // check result
    //--------------------------------------------------------------------------

    #ifdef GB_DEBUG
    ASSERT (Slice [0] == 0) ;
    ASSERT (Slice [ntasks] == n) ;
    for (int taskid = 0 ; taskid < ntasks ; taskid++)
    {
        ASSERT (Slice [taskid] <= Slice [taskid+1]) ;
    }
    #endif
}

