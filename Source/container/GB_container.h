//------------------------------------------------------------------------------
// GB_container.h: Container methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_CONTAINER_H
#define GB_CONTAINER_H

#include "GB.h"

void GB_vector_load
(
    // input/output:
    GrB_Vector V,           // vector to load from the C array X
    void **X,               // numerical array to load into V
    // input:
    uint64_t n,             // # of entries in X
    uint64_t X_size,        // size of X in bytes (at least n*(sizeof the type))
    GrB_Type type,          // type of X
    bool read_only          // if true, X is treated as read-only
) ;

GrB_Info GB_unload              // GrB_Matrix -> GxB_Container
(
    GrB_Matrix A,               // matrix to unload into the Container
    GxB_Container Container     // Container to hold the contents of A
) ;

GrB_Info GB_load                // GxB_Container -> GrB_Matrix
(
    GrB_Matrix A,               // matrix to load from the Container
    GxB_Container Container     // Container with contents to load into A
) ;

#endif

