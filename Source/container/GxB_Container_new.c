//------------------------------------------------------------------------------
// GxB_Container_new: create a new Container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_container.h"
#define GB_FREE_ALL GxB_Container_free (Container) ;

#define GB_VNEW(component, type)                                \
    GB_OK (GB_new ((GrB_Matrix *) (&((*Container)->component)), \
        type, 0, 1, GB_ph_null, true, GxB_FULL,                 \
        GB_HYPER_SWITCH_DEFAULT, 0, false, false, false)) ;     \
    (*Container)->component->magic = GB_MAGIC ;

//------------------------------------------------------------------------------
// GxB_Container_new
//------------------------------------------------------------------------------

GrB_Info GxB_Container_new
(
    GxB_Container *Container
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    GB_CHECK_INIT ;
    GB_RETURN_IF_NULL (Container) ;
    (*Container) = NULL ;

    //--------------------------------------------------------------------------
    // allocate the new Container
    //--------------------------------------------------------------------------

    size_t header_size ;
    (*Container) = GB_CALLOC_MEMORY (1, sizeof (struct GxB_Container_struct),
        &header_size) ;
    if (*Container == NULL)
    { 
        // out of memory
        return (GrB_OUT_OF_MEMORY) ;
    }
    (*Container)->header_size = header_size ;

    //--------------------------------------------------------------------------
    // allocate headers for the p, h, b, i and x components
    //--------------------------------------------------------------------------

    GB_VNEW (p, GrB_UINT32) ;
    GB_VNEW (h, GrB_UINT32) ;
    GB_VNEW (b, GrB_INT8) ;
    GB_VNEW (i, GrB_UINT32) ;
    GB_VNEW (x, GrB_BOOL) ;

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    return (GrB_SUCCESS) ;
}

