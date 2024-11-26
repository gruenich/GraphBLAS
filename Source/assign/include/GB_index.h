//------------------------------------------------------------------------------
// GB_index.h: definitions for index lists and types of assignments
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

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
// kind of index list, Ikind and Jkind, and assign variations
//------------------------------------------------------------------------------

#define GB_ALL 0
#define GB_RANGE 1
#define GB_STRIDE 2
#define GB_LIST 3

#define GB_ASSIGN 0
#define GB_SUBASSIGN 1
#define GB_ROW_ASSIGN 2
#define GB_COL_ASSIGN 3

#endif

