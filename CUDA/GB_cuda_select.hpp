//------------------------------------------------------------------------------
// GB_cuda_select.hpp: CPU definitions for CUDA select operations
//------------------------------------------------------------------------------

// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_CUDA_SELECT_H
#define GB_CUDA_SELECT_H

#include "GB_cuda.hpp"

//------------------------------------------------------------------------------
// GB_select_iso: assign the iso value of C for GB_*selector
//------------------------------------------------------------------------------

static inline void GB_select_iso
(
    GB_void *Cx,                    // output iso value (same type as A)
    const GB_Opcode opcode,         // selector opcode
    const GB_void *athunk,          // thunk scalar, of size asize
    const GB_void *Ax,              // Ax [0] scalar, of size asize
    const size_t asize
)
{
    if (opcode == GB_VALUEEQ_idxunop_code)
    { 
        // all entries in C are equal to thunk
        memcpy (Cx, athunk, asize) ;
    }
    else
    { 
        // A and C are both iso
        memcpy (Cx, Ax, asize) ;
    }
}

GrB_Info GB_cuda_select_bitmap_jit
(
    // output:
    int8_t *Cb,
    uint64_t *cnvals,
    // input:
    const bool C_iso,
    const GrB_Matrix A,
    const bool flipij,
    const GB_void *ythunk,
    const GrB_IndexUnaryOp op,
    // CUDA stream and launch parameters:
    cudaStream_t stream,
    int32_t gridsz,
    int32_t blocksz
) ;

GrB_Info GB_cuda_select_sparse_jit
(
    // output:
    GrB_Matrix C,
    // input:
    const bool C_iso,
    const GrB_Matrix A,
    const bool flipij,
    const GB_void *ythunk,
    const GrB_IndexUnaryOp op,
    // CUDA stream and launch parameters:
    cudaStream_t stream,
    int32_t gridsz,
    int32_t blocksz
) ;

#endif
