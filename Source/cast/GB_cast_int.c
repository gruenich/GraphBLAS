//------------------------------------------------------------------------------
// GB_cast_int: parallel memcpy or int32_t/int64_t/uint32_t/uint64_t type cast
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"

void GB_cast_int                // parallel memcpy/cast of integer arrays
(
    void *dest,                 // destination
    GB_Type_code dest_code,     // destination type: int32/64, or uint32/64
    const void *src,            // source
    GB_Type_code src_code,      // source type: int32/64, or uint32/64
    size_t n,                   // # of entries to copy
    int nthreads_max            // max # of threads to use
)
{

    //--------------------------------------------------------------------------
    // get the # of threads
    //--------------------------------------------------------------------------

    int nthreads = GB_nthreads (n, GB_CHUNK_DEFAULT, nthreads_max) ;
    int64_t k ;

    //--------------------------------------------------------------------------
    // copy/cast the integer array
    //--------------------------------------------------------------------------

    switch (dest_code)
    {

        //----------------------------------------------------------------------
        // destination is int32_t
        //----------------------------------------------------------------------

        case GB_INT32_code :

            switch (src_code)
            {
                case GB_INT32_code :
                case GB_UINT32_code : 
                    GB_memcpy (dest, src, n, nthreads) ;
                    break ;
                case GB_INT64_code :
                {
                    int32_t *Dest = (int32_t *) dest ;
                    int64_t *Src  = (int64_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                break ;
                case GB_UINT64_code :
                {
                    int32_t *Dest = (int32_t *) dest ;
                    int64_t *Src  = (uint64_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                default: ;
            }
            break ;

        //----------------------------------------------------------------------
        // destination is uint32_t
        //----------------------------------------------------------------------

        case GB_UINT32_code :

            switch (src_code)
            {
                case GB_INT32_code :
                case GB_UINT32_code : 
                    GB_memcpy (dest, src, n, nthreads) ;
                    break ;
                case GB_INT64_code :
                {
                    uint32_t *Dest = (uint32_t *) dest ;
                    int64_t  *Src  = (int64_t  *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                break ;
                case GB_UINT64_code :
                {
                    uint32_t *Dest = (uint32_t *) dest ;
                    int64_t *Src  = (uint64_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                break ;
                default: ;
            }
            break ;

        //----------------------------------------------------------------------
        // destination is int64_t
        //----------------------------------------------------------------------

        case GB_INT64_code :

            switch (src_code)
            {
                case GB_INT32_code :

                {
                    int64_t *Dest = (int64_t *) dest ;
                    int32_t *Src  = (int32_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                break ;

                case GB_UINT32_code : 
                {
                    int64_t  *Dest = (int64_t  *) dest ;
                    uint32_t *Src  = (uint32_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }

                case GB_INT64_code :
                case GB_UINT64_code :
                    GB_memcpy (dest, src, n, nthreads) ;
                    break ;
                default: ;
            }
            break ;

        //----------------------------------------------------------------------
        // destination is uint64_t
        //----------------------------------------------------------------------

        case GB_UINT64_code :

            switch (src_code)
            {
                case GB_INT32_code :

                {
                    uint64_t *Dest = (uint64_t *) dest ;
                    int32_t  *Src  = (int32_t  *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }
                break ;

                case GB_UINT32_code : 
                {
                    uint64_t *Dest = (uint64_t *) dest ;
                    uint32_t *Src  = (uint32_t *) src ;
                    #include "cast/factory/GB_cast_int_template.c"
                }

                case GB_INT64_code :
                case GB_UINT64_code :
                    GB_memcpy (dest, src, n, nthreads) ;
                    break ;
                default: ;
            }
            break ;

        default: ;
    }
}

