//------------------------------------------------------------------------------
// GB_jit_kernel.h:  JIT kernel #include for all kernels (both CPU and CUDA)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// This file is #include'd into all JIT kernels on the CPU and the GPU.

#ifndef GB_JIT_KERNEL_H
#define GB_JIT_KERNEL_H

#define GB_JIT_KERNEL

#ifndef GB_CUDA_KERNEL
    // for CPU JIT kernels:
    #include "include/GB_include.h"
#else
    // for CUDA JIT kernels:
    #include "include/GB_cuda_kernel.cuh"
#endif

// for all JIT kernels
#include "include/GB_jit_kernel_proto.h"
#if defined (_MSC_VER) && !(defined (__INTEL_COMPILER) || defined(__INTEL_CLANG_COMPILER))
    #define GB_JIT_GLOBAL extern __declspec ( dllexport )
#else
    #define GB_JIT_GLOBAL
#endif

#ifndef GB_JIT_RUNTIME
    // for PreJIT kernels (CPU and CUDA)
    #include "callback/GB_callbacks.h"
#endif

#undef GB_M_TYPE
#undef GB_MCAST

#endif

