//------------------------------------------------------------------------------
// GxB_Vector_load: load C array into a dense GrB_Vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// This method is guaranteed to take O(1) time and space.  If V starts as
// dense vector of length 0 with no content (V->x == NULL), then no malloc
// or frees are performed.

// If read_only is true, then V is created as a "shallow" vector.  Its
// numerical content, V->x = (*X), is "shallow" and thus treated as read-only
// by GraphBLAS.  It is not freed if V is freed with GrB_Vector_free.  (*X)
// need not be a malloc'd array at all.  Its allocation/deallocation is the
// responsibility of the user application.

// V is returned as a non-iso vector of length n, in the full data format.

#define GB_DEBUG

#include "GB.h"

GrB_Info GxB_Vector_load
(
    // input/output:
    GrB_Vector V,           // vector to load from the C array X
    void **X,               // numerical array to load into V
    // input:
    uint64_t n,             // # of entries in X
    uint64_t X_size,        // size of X in bytes (at least n*(sizeof the type))
    GrB_Type type,          // type of X
    bool read_only,         // if true, X is treated as read-only
    const GrB_Descriptor desc   // currently unused; for future expansion
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_RETURN_IF_NULL_OR_FAULTY (V) ;
    GB_RETURN_IF_NULL_OR_FAULTY (type) ;
    GB_RETURN_IF_NULL (X) ;
    GB_RETURN_IF_NULL (*X) ;
    if (X_size < n * type->size)
    { 
        // X is too small
        return (GrB_INVALID_VALUE) ;
    }
    ASSERT_VECTOR_OK (V, "V to load (contents mostly ignored)", GB0) ;

    //--------------------------------------------------------------------------
    // clear prior content of V and load X, making V a dense GrB_Vector
    //--------------------------------------------------------------------------

    // V->user_name is preserved; all other content is freed.  get/set controls
    // (hyper_switch, bitmap_switch, [pji]_control, etc) are preserved, except
    // that V->sparsity_control is revised to allow V to become a full vector.

    GB_phybix_free ((GrB_Matrix) V) ;
    V->type = type ;
    V->plen = -1 ;
    V->vlen = n ;
    V->vdim = 1 ;
    V->nvec = 1 ;
    V->nvec_nonempty = 1 ;
    V->nvals = n ;
    V->sparsity_control = V->sparsity_control | GxB_FULL ;
    V->is_csc = true ;
    V->jumbled = false ;
    V->iso = false ;
    V->p_is_32 = false ;
    V->j_is_32 = false ;
    V->i_is_32 = false ;

    //--------------------------------------------------------------------------
    // load the content into V
    //--------------------------------------------------------------------------

    V->x = (*X) ;
    V->x_shallow = read_only ;
    V->x_size = X_size ;
    if (!read_only)
    { 
        // tell the caller that X has been moved into V
        (*X) = NULL ;
    }

    // V is now a valid GrB_Vector of length n, in the full format
    V->magic = GB_MAGIC ;

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_VECTOR_OK (V, "V loaded", GB0) ;
    ASSERT (GB_IS_FULL (V)) ;
    return (GrB_SUCCESS) ;
}

