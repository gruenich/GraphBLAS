//------------------------------------------------------------------------------
// GB_msort_3: sort a 3-by-n list of integers, using A[0:2][ ] as the key
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// A parallel mergesort of an array of 3-by-n integers.  Each key
// consists of three integers.

#include "sort/GB_sort.h"
#define GB_msort_3_method GB_msort_3
#include "sort/factory/GB_msort_3_template.c"

