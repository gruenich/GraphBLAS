//------------------------------------------------------------------------------
// GxB_Descriptor_get: get a field in a descriptor
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Use GxB_Desc_get instead; this is kept for backward compatibility.

#include "GB.h"

GrB_Info GxB_Descriptor_get     // get a parameter from a descriptor
(
    int *val,                   // value of the parameter
    GrB_Descriptor desc,        // descriptor to query; NULL is ok
    int field                   // parameter to query
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_CHECK_INIT ;
    GB_RETURN_IF_NULL (val) ;
    GB_RETURN_IF_FAULTY (desc) ;

    //--------------------------------------------------------------------------
    // get the parameter
    //--------------------------------------------------------------------------

    switch (field)
    {
        case GrB_OUTP : 

            (*val) = (desc == NULL) ? GxB_DEFAULT : desc->out  ; break ;

        case GrB_MASK : 

            (*val) = (desc == NULL) ? GxB_DEFAULT : desc->mask ; break ;

        case GrB_INP0 : 

            (*val) = (desc == NULL) ? GxB_DEFAULT : desc->in0  ; break ;

        case GrB_INP1 : 

            (*val) = (desc == NULL) ? GxB_DEFAULT : desc->in1  ; break ;

        case GxB_AxB_METHOD : 

            (*val) = (desc == NULL) ? GxB_DEFAULT : desc->axb  ; break ;

        default : 

            return (GrB_INVALID_VALUE) ;
    }

    #pragma omp flush
    return (GrB_SUCCESS) ;
}

