//------------------------------------------------------------------------------
// GxB_Vector_unload: unload C array from a dense GrB_Vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// This method is guaranteed to take O(1) time and space, if on input V is a
// non-iso vector in the full data format.

// On input, V is a GrB_Vector with nvals(V) == length(V), in any data format.
// That is, all entries of V must be present.  If this condition does not hold,
// the method returns an error code, GrB_INVALID_OBJECT, indicating that the
// vector is not a valid input for this method.

// V is returned as valid full vector of length 0 with no content (V->x ==
// NULL).  It type is not changed.  If on input V was in the full data format,
// then no mallocs/frees are performed.

// If read_only is returned as true, then V was created as a "shallow" vector
// by GxB_Vector_load.  Its numerical content, V->x = (*X), was "shallow" and
// thus treated as read-only by GraphBLAS.  Its allocation/deallocation is the
// responsibility of the user application that created V via GxB_Vector_load.

// On output, *X is a pointer to the numerical contents of V.  If V had length
// zero on input, *X may be returned as a NULL pointer (which is not an error).

// This method removes X from the debug memtable, since X is being returned
// to the user application.

#include "GB_container.h"
#define GB_FREE_ALL ;

GrB_Info GxB_Vector_unload
(
    // input/output:
    GrB_Vector V,           // vector to unload
    void **X,               // numerical array to unload from V
    // output:
    uint64_t *n,            // # of entries in X
    uint64_t *X_size,       // size of X in bytes (at least n*(sizeof the type))
    GrB_Type *type,         // type of X
    bool *read_only,        // if true, X is treated as read-only
    const GrB_Descriptor desc   // currently unused; for future expansion
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_RETURN_IF_NULL_OR_FAULTY (V) ;
    GB_RETURN_IF_NULL (type) ;
    GB_RETURN_IF_NULL (X) ;
    GB_RETURN_IF_NULL (n) ;
    GB_RETURN_IF_NULL (X_size) ;
    GB_WHERE_1 (V, "GxB_Vector_unload") ;
    ASSERT_VECTOR_OK (V, "V to unload", GB0) ;

    //--------------------------------------------------------------------------
    // unload the vector
    //--------------------------------------------------------------------------

    GB_OK (GB_vector_unload (V, X, n, X_size, type, read_only, Werk)) ;
    GBMDUMP ("vector_unload, remove X from memtable %p\n", *X) ;
    GB_Global_memtable_remove (*X)  ;
    return (GrB_SUCCESS) ;
}

