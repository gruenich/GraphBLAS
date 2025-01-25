//------------------------------------------------------------------------------
// GB_hyper.h: definitions for hypersparse matrices and related methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_HYPER_H
#define GB_HYPER_H

int64_t GB_nvec_nonempty        // return # of non-empty vectors
(
    const GrB_Matrix A          // input matrix to examine
) ;

GrB_Info GB_hyper_realloc
(
    GrB_Matrix A,               // matrix with hyperlist to reallocate
    int64_t plen_new,           // new size of A->p and A->h
    GB_Werk Werk
) ;

GrB_Info GB_hyper_prune
(
    GrB_Matrix A,               // matrix to prune
    GB_Werk Werk
) ;

bool GB_hyper_hash_need         // return true if A needs a hyper hash
(
    GrB_Matrix A
) ;

GrB_Matrix GB_hyper_shallow         // return C
(
    GrB_Matrix C,                   // output matrix
    const GrB_Matrix A              // input matrix
) ;

#endif

