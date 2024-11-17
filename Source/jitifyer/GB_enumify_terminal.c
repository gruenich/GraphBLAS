//------------------------------------------------------------------------------
// GB_enumify_terminal: return enum of terminal value
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"
#include "jitifyer/GB_stringify.h"

void GB_enumify_terminal
(
    // output:
    int *ecode,             // enumerated terminal, 0 to 31
    // input:
    int add_ecode,          // add_ecode from GB_enumify_binop
    GB_Type_code zcode      // type code of the operator
)
{ 

    int e = 31 ;                // default is a non-terminal monoid

    switch (add_ecode)
    {

        // plus: non-terminal except for boolean lor
        case  9 :       // x + y, single complex
        case 10 :       // x + y, double complex
        case 11 :       // x + y

            // boolean PLUS (or) is terminal (true), others are not terminal
            e = (zcode == GB_BOOL_code) ? 2 : 31 ;
            break ;

        // times
        case 12 :       // x * y, single complex
        case 13 :       // x * y, double complex
        case 14 :       // x * y

            switch (zcode)
            {
                case GB_BOOL_code   : 
                    e = 3 ;             // false (boolean AND)
                    break ;
                case GB_INT8_code   : 
                case GB_INT16_code  : 
                case GB_INT32_code  : 
                case GB_INT64_code  : 
                case GB_UINT8_code  : 
                case GB_UINT16_code : 
                case GB_UINT32_code : 
                case GB_UINT64_code : 
                    e = 0 ;             // 0
                    break ;
                default: ;              // builtin with no terminal value
            }
            break ;

        // lor
        case 17 :       // x || y

                e = 2 ;                 // true
                break ;

        // land
        case 18 :       // x && y

                e = 3 ;                 // false
                break ;

        // min
        case  3 :       // fminf (x,y)
        case  4 :       // fmin (x,y)
        case  5 :       // GB_MIN (x,y)

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

        // max
        case  6 :       // fmaxf (x,y)
        case  7 :       // fmax (x,y)
        case  8 :       // GB_MAX (x,y)

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

        // any
        case  2 :           // any (x,y)

            e = 18 ;                    // no specific terminal value
            break ;

        // lxor, eq (lxnor), bxor, bxnor: non-terminal
        case 15 :       // x == y
        case 16 :       // x ^ y
        case 22 :       // bitwise xnor
        case 21 :       // bitwise xor
        default                     :   // builtin with no terminal value
            break ;

        // bor
        case 19 :       // bitwise or

            switch (zcode)
            {
                case GB_UINT8_code  : e = 19 ; break ; // 0xFF
                case GB_UINT16_code : e = 20 ; break ; // 0xFFFF
                case GB_UINT32_code : e = 21 ; break ; // 0xFFFFFFFF
                case GB_UINT64_code : e = 22 ; break ; // 0xFFFFFFFFFFFFFFFF
                default: ;
            }
            break ;

        // band
        case 20 :       // bitwise and

            e = 0  ;    // 0
            break ;

        // user-defined
        case 0 :        // user-defined monoid

            e = 30 ;
            break ;

    }

    (*ecode) = e ;
}

