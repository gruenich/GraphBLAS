//------------------------------------------------------------------------------
// GB_helper.h: helper functions for @GrB interface
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64

// These functions are only used by the @GrB interface for
// SuiteSparse:GraphBLAS.

#ifndef GB_HELPER_H
#define GB_HELPER_H

#include "GB.h"
#include "math/GB_math.h"

double GB_helper0 (void) ;

void GB_helper1              // convert zero-based indices to one-based
(
    double *restrict I_double,   // output array
    const uint64_t *restrict I,  // input array FIXME
    int64_t nvals                // size of input and output arrays
) ;

void GB_helper1i             // convert zero-based indices to one-based
(
    int64_t *restrict I,         // input/output array  FIXME
    int64_t nvals                // size of input/output array
) ;

bool GB_helper3              // return true if OK, false on error
(
    int64_t *restrict List,      // size len, output array  FIXME
    const double *restrict List_double, // size len, input array
    int64_t len,
    int64_t *List_max            // also compute the max entry in the list
) ;

bool GB_helper3i             // return true if OK, false on error
(
    int64_t *restrict List,      // size len, output array  FIXME
    const int64_t *restrict List_int64, // size len, input array    FIXME
    int64_t len,
    int64_t *List_max            // also compute the max entry in the list
) ;

bool GB_helper4              // return true if OK, false on error
(
    const uint64_t *restrict I,  // array of size len   FIXME
    const int64_t len,
    uint64_t *List_max           // find max (I) + 1
) ;

void GB_helper5              // construct pattern of S
(
    uint64_t *restrict Si,          // array of size anz    FIXME
    uint64_t *restrict Sj,          // array of size anz    FIXME
    const uint64_t *restrict Mi,    // array of size mnz, M->i  FIXME
    const uint64_t *restrict Mj,    // array of size mnz    FIXME
    const int64_t mvlen,            // M->vlen
    uint64_t *restrict Ai,          // array of size anz, A->i  FIXME
    const int64_t avlen,            // M->vlen
    const uint64_t anz
) ;

void GB_helper7              // Kx = uint64 (0:mnz-1)
(
    uint64_t *restrict Kx,      // array of size mnz    FIXME
    const uint64_t mnz
) ;

void GB_helper8
(
    GB_void *C,         // output array of size nvals * s
    GB_void *A,         // input scalar of size s
    uint64_t nvals,     // size of C
    size_t s            // size of each scalar
) ;

double GB_helper10       // norm (x-y,p), or -1 on error
(
    GB_void *x_arg,             // float or double, depending on type parameter
    bool x_iso,                 // true if x is iso
    GB_void *y_arg,             // same type as x, treat as zero if NULL
    bool y_iso,                 // true if x is iso
    GrB_Type type,              // GrB_FP32 or GrB_FP64
    int64_t p,                  // 0, 1, 2, INT64_MIN, or INT64_MAX
    uint64_t n
) ;

void GB_make_shallow (GrB_Matrix A) ;

#endif

