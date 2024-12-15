//------------------------------------------------------------------------------
// GB_apply_unop_ip: C = op (A), depending only on i
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 bit

// A can be jumbled.  If A is jumbled, so is C.

{

    //--------------------------------------------------------------------------
    // Cx = op (A)
    //--------------------------------------------------------------------------

    int64_t p ;
    #pragma omp parallel for num_threads(A_nthreads) schedule(static)
    for (p = 0 ; p < anz ; p++)
    { 
        if (!GBB_A (Ab, p)) continue ;  // FIXME
        // Cx [p] = op (A (i,j))
        GB_APPLY_OP (p, p) ;
    }
}

#undef GB_APPLY_OP

