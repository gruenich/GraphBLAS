//------------------------------------------------------------------------------
// GxB_load_Vector_from_Container: load a GrB_Vector from a Container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_container.h"
#define GB_FREE_ALL GB_phybix_free (V) ;

GrB_Info GxB_load_Vector_from_Container     // GrB_Vector <- GxB_Container
(
    GrB_Vector V,               // vector to load from the Container
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc   // currently unused
)
{
    GrB_Info info ;
    GB_OK (GB_load ((GrB_Matrix) V, Container)) ;
    GB_OK ((GB_VECTOR_OK (V) ? GrB_SUCCESS : GrB_INVALID_OBJECT)) ;
    return (GrB_SUCCESS) ;
}

