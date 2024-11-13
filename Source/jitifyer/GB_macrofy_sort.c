//------------------------------------------------------------------------------
// GB_macrofy_sort: construct all macros for sort methods
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"
#include "jitifyer/GB_stringify.h"

void GB_macrofy_sort            // construct all macros for GxB_sort
(
    // output:
    FILE *fp,                   // target file to write, already open
    // input:
    uint64_t scode,
    GrB_BinaryOp binaryop,      // binaryop to macrofy
    GrB_Type ctype
)
{ 

    //--------------------------------------------------------------------------
    // extract the binaryop scode
    //--------------------------------------------------------------------------

    // binary operator (3 hex digits)
    int binop_ecode = GB_RSHIFT (scode, 12, 8) ;
//  int xcode       = GB_RSHIFT (scode,  8, 4) ;

    // type of C (1 hex digit)
    int ccode       = GB_RSHIFT (scode,  0, 4) ;    // 1 to 14, C is not iso

    //--------------------------------------------------------------------------
    // describe the operator
    //--------------------------------------------------------------------------

    ASSERT_BINARYOP_OK (binaryop, "binaryop to macrofy", GB0) ;

    GrB_Type xtype = binaryop->xtype ;
    const char *xtype_name = xtype->name ;

    fprintf (fp, "// comparator: (%s, %s)\n\n", binaryop->name, xtype_name) ;

    //--------------------------------------------------------------------------
    // construct the typedefs
    //--------------------------------------------------------------------------

    GB_macrofy_typedefs (fp, ctype, xtype, NULL, NULL, NULL, NULL) ;

    fprintf (fp, "// comparator input type:\n") ;
    GB_macrofy_type (fp, "X", "_", xtype_name) ;

    //--------------------------------------------------------------------------
    // construct macros for the binary operator
    //--------------------------------------------------------------------------

    fprintf (fp, "\n// binary operator:\n") ;
    GB_macrofy_binop (fp, "GB_BINOP", false, false, false, true, false,
        binop_ecode, false, binaryop, NULL, NULL, NULL) ;

    //--------------------------------------------------------------------------
    // macros for the C matrix
    //--------------------------------------------------------------------------

    GB_macrofy_input (fp, "c", "C", "C", true, xtype, ctype, 1, ccode, 0, -1) ;

    //--------------------------------------------------------------------------
    // include the final default definitions
    //--------------------------------------------------------------------------

    fprintf (fp, "\n#include \"include/GB_kernel_shared_definitions.h\"\n") ;
}

