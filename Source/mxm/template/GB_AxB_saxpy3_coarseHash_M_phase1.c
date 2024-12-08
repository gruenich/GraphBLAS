//------------------------------------------------------------------------------
// GB_AxB_saxpy3_coarseHash_M_phase1: symbolic coarse hash method, with M
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

{
    // Initially, Hf [...] < mark for all of Hf.
    // Let h = Hi [hash] and f = Hf [hash].

    // f < mark: unoccupied, M(i,j)=0, C(i,j) ignored if
    //           this case occurs while scanning A(:,k)
    // h == i, f == mark   : M(i,j)=1, and C(i,j) not yet seen.
    // h == i, f == mark+1 : M(i,j)=1, and C(i,j) has been seen.

    for (int64_t kk = kfirst ; kk <= klast ; kk++)
    {
        GB_GET_B_j ;            // get B(:,j)
        Cp [kk] = 0 ;

        //----------------------------------------------------------------------
        // special case when B(:,j) is empty
        //----------------------------------------------------------------------

        #if ( GB_B_IS_SPARSE || GB_B_IS_HYPER )
        if (bjnz == 0) continue ;
        #endif

        //----------------------------------------------------------------------
        // get M(:,j) and scatter it into the Hf workspace
        //----------------------------------------------------------------------

        GB_GET_M_j ;                                // get M(:,j)
        if (mjnz == 0) continue ;
        GB_GET_M_j_RANGE (64) ;
        mark += 2 ;
        const int64_t f0 = mark ;
        const int64_t f1 = mark+1 ;
        GB_HASH_M_j ;                               // hash M(:,j)

        //----------------------------------------------------------------------
        // count nnz in C(:,j)
        //----------------------------------------------------------------------

        int64_t cjnz = 0 ;
        for ( ; pB < pB_end ; pB++)     // scan B(:,j)
        { 
            GB_GET_B_kj_INDEX ;         // get k of B(k,j)
            GB_GET_A_k ;                // get A(:,k)
            if (aknz == 0) continue ;
            #define GB_UPDATE_IKJ                                       \
            {                                                           \
                for (GB_HASH (i))               /* find i in hash */    \
                {                                                       \
                    int64_t f = Hf [hash] ;                             \
                    if (f < f0) break ;         /* M(i,j)=0; ignore */  \
                    if (Hi [hash] == i)         /* if true, i found */  \
                    {                                                   \
                        if (f == f0)            /* if true, i is new */ \
                        {                                               \
                            Hf [hash] = f1 ;    /* flag i as seen */    \
                            cjnz++ ;            /* C(i,j) is new */     \
                        }                                               \
                        break ;                                         \
                    }                                                   \
                }                                                       \
            }
            GB_SCAN_M_j_OR_A_k (((GB_A_IS_SPARSE || GB_A_IS_HYPER) && 
                !A_jumbled)) ;
            #undef GB_UPDATE_IKJ
        }
        Cp [kk] = cjnz ;                // count the entries in C(:,j)
    }
}

