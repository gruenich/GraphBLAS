//------------------------------------------------------------------------------
// gb_mxstring_to_string2: copy a built-in string into a C string
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// The string is optionally converted to lower case.  The string is mxMalloc'd
// and must be mxFree'd by the caller.

#include "gb_interface.h"

char *gb_mxstring_to_string2    // copy a built-in string into a C string
(
    const mxArray *S,       // built-in mxArray containing a string
    const char *name,       // name of the mxArray
    bool to_lower           // if true, convert to lower case
)
{ 

    char *string = NULL ;

    if (S != NULL && mxGetNumberOfElements (S) > 0)
    {
        if (!mxIsChar (S))
        { 
            ERROR2 ("%s must be a string", name) ;
        }
        size_t len = mxGetNumberOfElements (S) ;
        if (len > 0)
        {
            string = mxMalloc (len + 2) ;
            mxGetString (S, string, len + 2) ;
            string [len] = '\0' ;
            if (to_lower)
            { 
                // convert the string to lower case
                for (int k = 0 ; k < len && string [k] != '\0' ; k++)
                { 
                    string [k] = tolower (string [k]) ;
                }
            }
        }
    }

    if (string == NULL)
    { 
        string = mxMalloc (2) ;
        string [0] = '\0' ;
    }

    return (string) ;
}

