//------------------------------------------------------------------------------
// gbjit: control the GraphBLAS JIT
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: test GrB.jit

// Usage:

// [status,path] = gbjit
// [status,path] = gbjit (status)
// [status,path] = gbjit (status,path)

#include "gb_interface.h"

#define USAGE "usage: [status,path] = GrB.jit (status,path) ;"

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    gb_usage (nargin <= 2 && nargout <= 2, USAGE) ;

    //--------------------------------------------------------------------------
    // set the JIT control, if requested
    //--------------------------------------------------------------------------

    if (nargin > 0)
    { 
        // set the JIT control
        #define JIT(c) \
            OK (GrB_Global_set_INT32 (GrB_GLOBAL, c, GxB_JIT_C_CONTROL)) ;
        char *status = gb_mxstring_to_string2 (pargin [0], "status", true) ;
        if      (MATCH (status, ""     ))
        { 
            /* do nothing */ ;
        }
        else if (MATCH (status, "off"  ))
        { 
            JIT (GxB_JIT_OFF) ;
        }
        else if (MATCH (status, "pause"))
        { 
            JIT (GxB_JIT_PAUSE) ;
        }
        else if (MATCH (status, "run"  ))
        { 
            JIT (GxB_JIT_RUN) ;
        }
        else if (MATCH (status, "load" ))
        { 
            JIT (GxB_JIT_LOAD) ;
        }
        else if (MATCH (status, "on"   ))
        { 
            JIT (GxB_JIT_ON) ;
        }
        else if (MATCH (status, "flush"))
        { 
            JIT (GxB_JIT_OFF) ;
            JIT (GxB_JIT_ON) ;
        }
        else
        { 
            ERROR2 ("unknown option: %s", status) ;
        }
        mxFree (status) ;
    }

    //--------------------------------------------------------------------------
    // set the cache path, if requested
    //--------------------------------------------------------------------------

    if (nargin > 1)
    { 
        // set the JIT cache path
        char *path = gb_mxstring_to_string2 (pargin [1], "path", false) ;
        OK (GrB_Global_set_String (GrB_GLOBAL, path, GxB_JIT_CACHE_PATH)) ;
        mxFree (path) ;
    }

    //--------------------------------------------------------------------------
    // get the JIT control, if requested
    //--------------------------------------------------------------------------

    if (nargout > 0)
    { 
        int c ;
        OK (GrB_Global_get_INT32 (GrB_GLOBAL, &c, GxB_JIT_C_CONTROL)) ;
        switch (c)
        {
            case GxB_JIT_OFF  : pargout [0] = mxCreateString ("off"  ) ; break ;
            case GxB_JIT_PAUSE: pargout [0] = mxCreateString ("pause") ; break ;
            case GxB_JIT_RUN  : pargout [0] = mxCreateString ("run"  ) ; break ;
            case GxB_JIT_LOAD : pargout [0] = mxCreateString ("load" ) ; break ;
            case GxB_JIT_ON   : pargout [0] = mxCreateString ("on"   ) ; break ;
            default           : pargout [0] = mxCreateString ("unknown") ;
                                break ;
        }
    }

    //--------------------------------------------------------------------------
    // get the JIT cache path, if requested
    //--------------------------------------------------------------------------

    if (nargout > 1)
    { 
        char *path = NULL ;
        size_t len = 0 ;
        OK (GrB_Global_get_SIZE (GrB_GLOBAL, &len, GxB_JIT_CACHE_PATH)) ;
        path = mxMalloc (len + 2) ;
        OK (GrB_Global_get_String (GrB_GLOBAL, path, GxB_JIT_CACHE_PATH)) ;
        pargout [1] = mxCreateString (path) ;
        mxFree (path) ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    GB_WRAPUP ;
}

