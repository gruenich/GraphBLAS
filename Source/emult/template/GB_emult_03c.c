//------------------------------------------------------------------------------
// GB_emult_03c: C<#M>=A.*B when M and A are bitmap/full and B is sparse/hyper
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// C is sparse, with the same sparsity structure as B.
// M is bitmap/full.  A is bitmap/full, and B is sparse/hyper.

{

    //--------------------------------------------------------------------------
    // Method3(c): C<#M>=A.*B; M and A are bitmap/full; B is sparse/hyper
    //--------------------------------------------------------------------------

    const int8_t *restrict Mb = M->b ;
    const GB_M_TYPE *restrict Mx = (Mask_struct) ? NULL : ((GB_M_TYPE *) M->x) ;
    const size_t msize = M->type->size ;

    int tid ;
    #pragma omp parallel for num_threads(B_nthreads) schedule(dynamic,1)
    for (tid = 0 ; tid < B_ntasks ; tid++)
    {
        int64_t kfirst = kfirst_Bslice [tid] ;
        int64_t klast  = klast_Bslice  [tid] ;
        for (int64_t k = kfirst ; k <= klast ; k++)
        {
            int64_t j = GBh_B (Bh, k) ;
            int64_t pA_start = j * vlen ;
            GB_GET_PA_AND_PC (pB, pB_end, pC, tid, k, kfirst, klast,
                pstart_Bslice, Cp_kfirst,
                GB_IGET (Bp, k), GB_IGET (Bp, k+1), GB_IGET (Cp, k)) ;
            for ( ; pB < pB_end ; pB++)
            { 
                int64_t i = GB_IGET (Bi, pB) ;
                int64_t pA = pA_start + i ;
                if (!GBb_A (Ab, pA)) continue ;
                bool mij = GBb_M (Mb, pA) && GB_MCAST (Mx, pA, msize) ;
                mij = mij ^ Mask_comp ;
                if (!mij) continue ;
                // C (i,j) = A (i,j) .* B (i,j)
                GB_ISET (Ci, pC, i) ;   // Ci [pC] = i ;
                #ifndef GB_ISO_EMULT
                GB_DECLAREA (aij) ;
                GB_GETA (aij, Ax, pA, A_iso) ;     
                GB_DECLAREB (bij) ;
                GB_GETB (bij, Bx, pB, B_iso) ;
                GB_EWISEOP (Cx, pC, aij, bij, i, j) ;
                #endif
                pC++ ;
            }
        }
    }
}

