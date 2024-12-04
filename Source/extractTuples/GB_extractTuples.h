//------------------------------------------------------------------------------
// GB_extractTuples.h: definitions for GB_extractTuples and related methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#ifndef GB_EXTRACTTUPLES_H
#define GB_EXTRACTTUPLES_H

GrB_Info GB_extract_vector_list // extract vector list from a matrix
(
    // output:
    void *J,                    // size nnz(A) or more
    // input:
    bool is_32,                 // if true, J is 32-bit; else 64-bit
    const GrB_Matrix A,
    GB_Werk Werk
) ;

GrB_Info GB_extractTuples       // extract all tuples from a matrix
(
    void *I_out,                // array for returning row indices of tuples
    void *J_out,                // array for returning col indices of tuples
    void *X,                    // array for returning values of tuples
    uint64_t *p_nvals,          // I,J,X size on input; # tuples on output
    const GrB_Type xtype,       // type of array X
    const GrB_Matrix A,         // matrix to extract tuples from
    bool is_32,                 // if true, I and J are 32-bit; else 64-bit
    GB_Werk Werk
) ;

#endif

