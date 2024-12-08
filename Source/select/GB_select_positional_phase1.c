//------------------------------------------------------------------------------
// GB_select_positional_phase1: count entries for C=select(A,thunk)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit

// A is sparse or hypersparse

// JIT: not need: use factory; 3 variants (A sparse, hyper, or full for DIAG)

#include "select/GB_select.h"

GrB_Info GB_select_positional_phase1
(
    // input/output:
    GrB_Matrix C,
    // output:
    uint64_t *restrict Zp,      // FIXME
//  void *Zp,                       // if C->p_is_32: 32 bit, else 64-bit
    uint64_t *restrict Wfirst,
    uint64_t *restrict Wlast,
    // input:
    const GrB_Matrix A,
    const int64_t ithunk,
    const GrB_IndexUnaryOp op,
    const int64_t *A_ek_slicing,
    const int A_ntasks,
    const int A_nthreads
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_Opcode opcode = op->opcode ;
    ASSERT (GB_IS_SPARSE (A) || GB_IS_HYPERSPARSE (A)
        || (opcode == GB_DIAG_idxunop_code)) ;
    ASSERT (GB_IS_INDEXUNARYOP_CODE_POSITIONAL (opcode)
        || opcode == GB_NONZOMBIE_idxunop_code) ;
    ASSERT (!GB_IS_BITMAP (A)) ;

    uint64_t *restrict Cp = C->p ;  // FIXME

    //--------------------------------------------------------------------------
    // phase1: positional operators and nonzombie selector
    //--------------------------------------------------------------------------

    #include "select/include/GB_select_shared_definitions.h"

    switch (opcode)
    {

        case GB_TRIL_idxunop_code      : 
            #define GB_TRIL_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_TRIU_idxunop_code      : 
            #define GB_TRIU_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_DIAG_idxunop_code      : 
            #define GB_DIAG_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_OFFDIAG_idxunop_code   : 
        case GB_DIAGINDEX_idxunop_code : 
            #define GB_OFFDIAG_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_ROWINDEX_idxunop_code  : 
            #define GB_ROWINDEX_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_ROWLE_idxunop_code     : 
            #define GB_ROWLE_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_ROWGT_idxunop_code     : 
            #define GB_ROWGT_SELECTOR
            #include "select/factory/GB_select_positional_phase1_template.c"
            break ;

        case GB_NONZOMBIE_idxunop_code : 
            // keep A(i,j) if it's not a zombie
            #define GB_A_TYPE GB_void
            #define GB_TEST_VALUE_OF_ENTRY(keep,p) bool keep = (i >= 0)
            #include "select/template/GB_select_entry_phase1_template.c"
            break ;

        default: ;
    }

    return (GrB_SUCCESS) ;
}

