//------------------------------------------------------------------------------
// GB_search_for_vector: find the vector k that contains p
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// Given an index p, find k so that Ap [k] <= p && p < Ap [k+1].  The search is
// limited to k in the range Ap [kleft ... anvec].

// A->p can come from any matrix: hypersparse, sparse, bitmap, or full.
// For the latter two cases, A->p is NULL.

#ifndef GB_SEARCH_FOR_VECTOR_H
#define GB_SEARCH_FOR_VECTOR_H

#define GB_SV_TYPE uint32_t
#define GB_search_for_vector_TYPE GB_search_for_vector_32
#include "include/GB_search_for_vector_template.h"

#define GB_SV_TYPE uint64_t
#define GB_search_for_vector_TYPE GB_search_for_vector_64
#include "include/GB_search_for_vector_template.h"

#endif

