//------------------------------------------------------------------------------
// GB_bld:  hard-coded functions for builder methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_control.h"
#if defined (GxB_NO_FP32)
#define GB_TYPE_ENABLED 0
#else
#define GB_TYPE_ENABLED 1
#endif

#if GB_TYPE_ENABLED
#include "GB.h"
#include "FactoryKernels/GB_bld__include.h"

// dup operator: Tx [k] += Sx [i], no typecast here
#define GB_BLD_DUP(Tx,k,Sx,i)  Tx [k] *= Sx [i]
#define GB_BLD_COPY(Tx,k,Sx,i) Tx [k] = Sx [i]

// array types for S and T
#define GB_Sx_TYPE float
#define GB_Tx_TYPE float

// operator types: z = dup (x,y)
#define GB_Z_TYPE  float
#define GB_X_TYPE  float
#define GB_Y_TYPE  float

// disable this operator and use the generic case if these conditions hold
#if (defined(GxB_NO_TIMES) || defined(GxB_NO_FP32) || defined(GxB_NO_TIMES_FP32))
#define GB_DISABLE 1
#else
#define GB_DISABLE 0
#endif

#include "omp/include/GB_kernel_shared_definitions.h"

//------------------------------------------------------------------------------
// build a non-iso matrix
//------------------------------------------------------------------------------

GrB_Info GB (_bld__times_fp32)
(
    GB_Tx_TYPE *restrict Tx,
    void *restrict Ti,
    bool Ti_is_32,
    const GB_Sx_TYPE *restrict Sx,
    int64_t nvals,
    int64_t ndupl,
    const void *restrict I_work,
    bool I_is_32,
    const void *restrict K_work,
    bool K_is_32,
    const int64_t *restrict tstart_slice,
    const int64_t *restrict tnz_slice,
    int nthreads
)
{ 
    #if GB_DISABLE
    return (GrB_NO_VALUE) ;
    #else
    const int32_t *restrict I_work32 = (I_is_32) ? I_work : NULL ;
    const int64_t *restrict I_work64 = (I_is_32) ? NULL : I_work ;
    const uint32_t *restrict K_work32 = (K_is_32) ? K_work : NULL ;
    const uint64_t *restrict K_work64 = (K_is_32) ? NULL : K_work ;
    int32_t *restrict Ti32 = (Ti_is_32) ? Ti : NULL ;
    int64_t *restrict Ti64 = (Ti_is_32) ? NULL : Ti ;
    #define GB_K_WORK(t) (K_work ? GB_IGET (K_work, t) : (t))
    #include "builder/template/GB_bld_template.c"
    return (GrB_SUCCESS) ;
    #endif
}

#endif

