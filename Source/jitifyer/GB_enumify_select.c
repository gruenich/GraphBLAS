//------------------------------------------------------------------------------
// GB_enumify_select: enumerate a GrB_select problem
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit, except for trimming out 3 bits for idxop_ecode

#include "GB.h"
#include "jitifyer/GB_stringify.h"

// Currently, the mask M and the accum are not present, and C and A have the
// same type, but these conditions may change in the future.

void GB_enumify_select      // enumerate a GrB_selectproblem
(
    // output:
    uint64_t *method_code,  // unique encoding of the entire operation
    // input:
    const GrB_Matrix C,
    const GrB_IndexUnaryOp op,   // the index unary operator to enumify
    const bool flipij,           // if true, flip i and j
    const GrB_Matrix A
)
{

    //--------------------------------------------------------------------------
    // get the types of A, X, Y, and Z
    //--------------------------------------------------------------------------

    GrB_Type atype = A->type ;
    GB_Opcode opcode = op->opcode ;
    GB_Type_code zcode = op->ztype->code ;
    GB_Type_code xcode = (op->xtype == NULL) ? 0 : op->xtype->code ;
    GB_Type_code ycode = op->ytype->code ;

    //--------------------------------------------------------------------------
    // enumify the idxunop operator
    //--------------------------------------------------------------------------

    bool depends_on_x, depends_on_i, depends_on_j, depends_on_y ;
    int idxop_ecode ;
    GB_enumify_unop (&idxop_ecode, &depends_on_x, &depends_on_i,
        &depends_on_j, &depends_on_y, flipij, opcode, xcode) ;

    ASSERT (idxop_ecode >= 231 && idxop_ecode <= 254) ;

    if (!depends_on_x)
    { 
        // VALUE* ops and user-defined index unary ops depend on x.  The
        // positional ops (tril, triu, row*, col*, diag*) do not.
        xcode = 0 ;
    }

    if (!depends_on_y)
    { 
        // All index unary ops depend on y except for NONZOMBIE
        ycode = 0 ;
    }

    int i_dep = (depends_on_i) ? 1 : 0 ;
    int j_dep = (depends_on_j) ? 1 : 0 ;

    //--------------------------------------------------------------------------
    // enumify the types
    //--------------------------------------------------------------------------

    int acode = atype->code ;               // 1 to 14
    int ccode = acode ;                     // this may change in the future
    int A_iso_code = (A->iso) ? 1 : 0 ;
    int C_iso_code = (C->iso) ? 1 : 0 ;

    //--------------------------------------------------------------------------
    // enumify the sparsity structure of A and C
    //--------------------------------------------------------------------------

    int asparsity, csparsity ;
    GB_enumify_sparsity (&csparsity, GB_sparsity (C)) ;
    GB_enumify_sparsity (&asparsity, GB_sparsity (A)) ;

    int cp_is_32 = (C->p_is_32) ? 1 : 0 ;
    int ci_is_32 = (C->i_is_32) ? 1 : 0 ;
    int ap_is_32 = (A->p_is_32) ? 1 : 0 ;
    int ai_is_32 = (A->i_is_32) ? 1 : 0 ;

    //--------------------------------------------------------------------------
    // construct the select method_code
    //--------------------------------------------------------------------------

    // total method_code bits:  41 (11 hex digits): can reduce to 38 bits

    // FIXME : reduce bits for idxop_ecode, by subtracting 231: then 0 to 23,
    // which is 5 bits, not 8 bits

    (*method_code) =
                                               // range        bits

                // C, A: 32/64 (4 bits) (1 hex digit)
                GB_LSHIFT (cp_is_32   , 43) |  // 0 or 1       1
                GB_LSHIFT (ci_is_32   , 42) |  // 0 or 1       1
                GB_LSHIFT (ap_is_32   , 41) |  // 0 or 1       1
                GB_LSHIFT (ai_is_32   , 40) |  // 0 or 1       1

                // iso of A and C (2 bits, 1 hex digit)
                // 2 bits unused
                GB_LSHIFT (C_iso_code , 37) |  // 0 or 1       1
                GB_LSHIFT (A_iso_code , 36) |  // 0 or 1       1

                // i/j dependency and flipij (1 hex digit)
                // one bit unused
                GB_LSHIFT (i_dep      , 34) |  // 0 or 1       1
                GB_LSHIFT (j_dep      , 33) |  // 0 or 1       1
                GB_LSHIFT (flipij     , 32) |  // 0 or 1       1

                // op, z = f(x,i,j,y) (5 hex digits)
                GB_LSHIFT (idxop_ecode, 24) |  // 231 to 254   8
                GB_LSHIFT (zcode      , 20) |  // 0 to 14      4
                GB_LSHIFT (xcode      , 16) |  // 0 to 14      4
                GB_LSHIFT (ycode      , 12) |  // 0 to 14      4

                // types of C and A (2 hex digits)
                GB_LSHIFT (ccode      ,  8) |  // 0 to 15      4    // == acode
                GB_LSHIFT (acode      ,  4) |  // 0 to 15      4

                // sparsity structures of C and A (1 hex digit)
                GB_LSHIFT (csparsity  ,  2) |  // 0 to 3       2
                GB_LSHIFT (asparsity  ,  0) ;  // 0 to 3       2
}

