//------------------------------------------------------------------------------
// GB_enumify_identity: return ecode for identity value of an op of a monoid
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"
#include "jitifyer/GB_stringify.h"

void GB_enumify_identity
(
    // output:
    int *ecode,             // enumerated identity, 0 to 31
    // inputs:
    int add_ecode,          // add_ecode from GB_enumify_binop
    GB_Type_code zcode      // type code of the operator
)
{ 

    int e = 31 ;            // default: use the monoid->identity bytes

    switch (add_ecode)
    {

        // plus
        case  9 :       // x + y, single complex
        case 10 :       // x + y, double complex
        case 11 :       // x + y
            e = 0 ;     // identity is 0
            break ;

        // times
        case 12 :       // x * y, single complex
        case 13 :       // x * y, double complex
        case 14 :       // x * y
            e = 1 ;     // identity is 1
            break ;

        // land and eq (lxnor)
        case 18 :       // x && y
        case 15 :       // x == y
            e = (zcode == GB_BOOL_code) ? 2 : (31) ; // true
            break ;

        // lor and lxor
        case 17 :       // x || y
        case 16 :       // x ^ y
            e = (zcode == GB_BOOL_code) ? 3 : (31) ; // false
            break ;

        // min
        case  3 :       // fminf (x,y)
        case  4 :       // fmin (x,y)
        case  5 :       // GB_MIN (x,y)
            switch (zcode)
            {
                case GB_BOOL_code   : e =  2 ; break ; // true
                case GB_INT8_code   : e =  4 ; break ; // INT8_MAX
                case GB_INT16_code  : e =  5 ; break ; // INT16_MAX
                case GB_INT32_code  : e =  6 ; break ; // INT32_MAX
                case GB_INT64_code  : e =  7 ; break ; // INT64_MAX
                case GB_UINT8_code  : e =  8 ; break ; // UINT8_MAX
                case GB_UINT16_code : e =  9 ; break ; // UINT16_MAX
                case GB_UINT32_code : e = 10 ; break ; // UINT32_MAX
                case GB_UINT64_code : e = 11 ; break ; // UINT64_MAX
                case GB_FP32_code   : 
                case GB_FP64_code   : e = 12 ; break ; // INFINITY
                default: ;
            }
            break ;

        // max
        case  6 :       // fmaxf (x,y)
        case  7 :       // fmax (x,y)
        case  8 :       // GB_MAX (x,y)
            switch (zcode)
            {
                case GB_BOOL_code   : e =  3 ; break ; // false
                case GB_INT8_code   : e = 13 ; break ; // INT8_MIN
                case GB_INT16_code  : e = 14 ; break ; // INT16_MIN
                case GB_INT32_code  : e = 15 ; break ; // INT32_MIN
                case GB_INT64_code  : e = 16 ; break ; // INT64_MIN
                case GB_UINT8_code  : 
                case GB_UINT16_code : 
                case GB_UINT32_code : 
                case GB_UINT64_code : e =  0 ; break ; // 0
                case GB_FP32_code   : 
                case GB_FP64_code   : e = 17 ; break ; // -INFINITY
                default: ;
            }
            break ;

        // any
        case  1 :       // first (x,y)
        case  2 :       // any (x,y)
            e = 18 ;    // 0, for ANY op only
            break ;

        // bor and bxor
        case 19 :       // bitwise or
        case 21 :       // bitwise xor
            e = 0 ;     // 0
            break ;

        // band and bxnor
        case 20 :       // bitwise and
        case 22 :       // bitwise xnor
            switch (zcode)
            {
                case GB_UINT8_code  : e = 19 ; break ; // 0xFF
                case GB_UINT16_code : e = 20 ; break ; // 0xFFFF
                case GB_UINT32_code : e = 21 ; break ; // 0xFFFFFFFF
                case GB_UINT64_code : e = 22 ; break ; // 0xFFFFFFFFFFFFFFFF
                default: ;
            }
            break ;

        default: ;
    }

    (*ecode) = e ;
}

