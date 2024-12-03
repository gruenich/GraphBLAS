//------------------------------------------------------------------------------
// GB_extractTuples.h: definitions for GB_extractTuples and related methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit

#ifndef GB_EXTRACTTUPLES_H
#define GB_EXTRACTTUPLES_H

GrB_Info GB_extract_vector_list     // extract vector list from a matrix
(
    // output:
    int64_t *restrict J,         // size nnz(A) or more FIXME
    // input:
    const GrB_Matrix A,
    GB_Werk Werk
) ;

GrB_Info GB_extractTuples       // extract all tuples from a matrix
(
    GrB_Index *I_out, /*FIXME*/ // array for returning row indices of tuples
    GrB_Index *J_out, /*FIXME*/ // array for returning col indices of tuples
    void *X,                    // array for returning values of tuples
    GrB_Index *p_nvals,         // I,J,X size on input; # tuples on output
    const GrB_Type xtype,       // type of array X
    const GrB_Matrix A,         // matrix to extract tuples from
    GB_Werk Werk
) ;

#endif

