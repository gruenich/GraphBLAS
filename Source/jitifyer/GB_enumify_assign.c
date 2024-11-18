//------------------------------------------------------------------------------
// GB_enumify_assign: enumerate a GrB_assign problem
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#define GB_DEBUG    /* HACK FIXME */

// Enumify an assign/subassign operation: C(I,J)<M> += A.  No transpose is
// handled; this is done first in GB_assign_prep.

// The user-callable methods, GrB_assign and GxB_subassign and their variants,
// call GB_assign and GB_subassign, respectively.  Both of those call either
// GB_bitmap_assign or GB_subassigner to do the actual work, or related methods
// that do not need a JIT (GB_*assign_zombie, in particular).

// GB_bitmap_assign and GB_subassigner do not call the JIT directly.  Instead,
// they call one of the many assign/subassign kernels, each of which has a JIT
// variant.

#include "GB.h"
#include "jitifyer/GB_stringify.h"

void GB_enumify_assign      // enumerate a GrB_assign problem
(
    // output:
    uint64_t *scode,        // unique encoding of the entire operation
    // input:
    // C matrix:
    GrB_Matrix C,
    bool C_replace,
    // index types:
    int Ikind,              // 0: all (no I), 1: range, 2: stride, 3: list
    int Jkind,              // ditto
    // M matrix:
    GrB_Matrix M,           // may be NULL
    bool Mask_comp,         // mask is complemented
    bool Mask_struct,       // mask is structural
    // operator:
    GrB_BinaryOp accum,     // the accum operator (may be NULL)
    // A matrix or scalar
    GrB_Matrix A,           // NULL for scalar assignment
    GrB_Type scalar_type,
    // S matrix:
    GrB_Matrix S,           // may be MULL
    int assign_kind         // 0: assign, 1: subassign, 2: row, 3: col
)
{

    //--------------------------------------------------------------------------
    // get the types of C, M, and A (or the scalar)
    //--------------------------------------------------------------------------

    GrB_Type ctype = C->type ;
    GrB_Type mtype = (M == NULL) ? NULL : M->type ;
    GrB_Type atype = (A == NULL) ? scalar_type : A->type ;
    ASSERT (atype != NULL) ;

    //--------------------------------------------------------------------------
    // get the types of X, Y, and Z
    //--------------------------------------------------------------------------

    GB_Opcode accum_opcode ;
    GB_Type_code xcode, ycode, zcode ;

    if (accum == NULL)
    { 
        // accum is not present
        accum_opcode = GB_NOP_code ;
        xcode = 0 ;
        ycode = 0 ;
        zcode = 0 ;
    }
    else
    { 
        accum_opcode = accum->opcode ;
        xcode = accum->xtype->code ;
        ycode = accum->ytype->code ;
        zcode = accum->ztype->code ;
        if (xcode == GB_BOOL_code)  // && (ycode == GB_BOOL_code)
        { 
            // rename the operator
            accum_opcode = GB_boolean_rename (accum_opcode) ;
        }
    }

    //--------------------------------------------------------------------------
    // enumify the accum operator, if present
    //--------------------------------------------------------------------------

    // accum_ecode is 255 if no accum is present

    // FIXME: replace with ecode = (opcode-GB_USER_binop_code) & 0x2F (6 bits),
    // and do the GB_enumify_binop in the macrofy stage.

    int accum_ecode ;
    GB_enumify_binop (&accum_ecode, accum_opcode, xcode, false, false) ;

    //--------------------------------------------------------------------------
    // enumify the types
    //--------------------------------------------------------------------------

    int acode = atype->code ;                           // 1 to 14
    int A_iso_code = (A != NULL && A->iso) ? 1 : 0 ;
    int s_assign = (A == NULL) ? 1 : 0 ;                // scalar assignment

    // if (ccode == 0): C is iso and the kernel does not access its values
    int ccode = (C->iso) ? 0 : ctype->code ;            // 0 to 14

    //--------------------------------------------------------------------------
    // enumify the mask
    //--------------------------------------------------------------------------

    // mtype_code == 0: no mask present
    int mtype_code = (mtype == NULL) ? 0 : mtype->code ; // 0 to 14
    int mask_ecode ;
    GB_enumify_mask (&mask_ecode, mtype_code, Mask_struct, Mask_comp) ;

    //--------------------------------------------------------------------------
    // enumify the sparsity structures of C, M, A, and B
    //--------------------------------------------------------------------------

    int C_sparsity = GB_sparsity (C) ;
    int M_sparsity = (M == NULL) ? 0 : GB_sparsity (M) ;
    int A_sparsity = (A == NULL) ? 0 : GB_sparsity (A) ;
    int S_sparsity = (S == NULL) ? 0 : GB_sparsity (S) ;
    int S_present = (S != NULL) ;

    int csparsity, msparsity, asparsity, ssparsity ;
    GB_enumify_sparsity (&csparsity, C_sparsity) ;
    GB_enumify_sparsity (&msparsity, M_sparsity) ;
    GB_enumify_sparsity (&asparsity, A_sparsity) ;
    GB_enumify_sparsity (&ssparsity, S_sparsity) ;

    int C_repl = (C_replace) ? 1 : 0 ;

    //--------------------------------------------------------------------------
    // construct the assign scode
    //--------------------------------------------------------------------------

    // total scode bits: 50 (13 hex digits)

    (*scode) =
                                               // range        bits
                /// sparsity of S (1 hex digit)
                GB_LSHIFT (ssparsity  , 48) |  // 0 to 3       2

                // assign_kind, Ikind, Jkind, S present (2 hex digits)
                GB_LSHIFT (S_present  , 47) |  // 0 to 1       1
                GB_LSHIFT (C_repl     , 46) |  // 0 to 1       1
                GB_LSHIFT (assign_kind, 44) |  // 0 to 3       2
                GB_LSHIFT (Ikind      , 42) |  // 0 to 3       2
                GB_LSHIFT (Jkind      , 40) |  // 0 to 3       2

                // accum, z = f(x,y) (5 hex digits)
                GB_LSHIFT (accum_ecode, 32) |  // 0 to 255     8
                GB_LSHIFT (zcode      , 28) |  // 0 to 14      4
                GB_LSHIFT (xcode      , 24) |  // 0 to 14      4
                GB_LSHIFT (ycode      , 20) |  // 0 to 14      4

                // mask (one hex digit)
                GB_LSHIFT (mask_ecode , 16) |  // 0 to 13      4

                // types of C and A (or scalar type) (2 hex digits)
                GB_LSHIFT (ccode      , 12) |  // 0 to 14      4
                GB_LSHIFT (acode      ,  8) |  // 1 to 14      4

                // sparsity structures of C, M, and A (2 hex digits),
                // iso status of A and scalar assignment
                GB_LSHIFT (csparsity  ,  6) |  // 0 to 3       2
                GB_LSHIFT (msparsity  ,  4) |  // 0 to 3       2
                GB_LSHIFT (s_assign   ,  3) |  // 0 to 1       1
                GB_LSHIFT (A_iso_code ,  2) |  // 0 or 1       1
                GB_LSHIFT (asparsity  ,  0) ;  // 0 to 3       2

}

