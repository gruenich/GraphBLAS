//------------------------------------------------------------------------------
// GB_AxB_dot4_cij.c: C(i,j) = A(:,i)'*B(:,j) for dot4 method
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// A is sparse or hypersparse, B is full or bitmap, and C is full

{

    //--------------------------------------------------------------------------
    // get C(i,j)
    //--------------------------------------------------------------------------

    const int64_t pC = i + pC_start ;   // C(i,j) is at Cx [pC]
    GB_C_TYPE GB_GET4C (cij, pC) ;       // cij = Cx [pC]

    //--------------------------------------------------------------------------
    // C(i,j) += A (:,i)*B(:,j): a single dot product
    //--------------------------------------------------------------------------

    #if ( GB_B_IS_FULL )
    {

        //----------------------------------------------------------------------
        // A is sparse/hyper and B is full
        //----------------------------------------------------------------------

        #if GB_IS_EQ_PAIR_SEMIRING
        { 
            // (boolean EQ (LXNOR) monoid)_PAIR semiring
            cij = (cij == 1) ;
        }
        #elif GB_IS_XOR_PAIR_SEMIRING
        { 
            // (boolean XOR monoid)_PAIR semiring
            uint64_t t = ((uint64_t) cij) + ainz ;
            cij = (GB_C_TYPE) (t & 0x1L) ;
        }
        #elif GB_IS_PLUS_8_PAIR_SEMIRING
        { 
            // (PLUS int8, uint8 monoids)_PAIR semirings
            uint64_t t = ((uint64_t) cij) + ainz ;
            cij = (GB_C_TYPE) (t & 0xFFL) ;
        }
        #elif GB_IS_PLUS_16_PAIR_SEMIRING
        { 
            // (PLUS int16, uint16 monoids)_PAIR semirings
            uint64_t t = ((uint64_t) cij) + ainz ;
            cij = (GB_C_TYPE) (t & 0xFFFFL) ;
        }
        #elif GB_IS_PLUS_32_PAIR_SEMIRING
        { 
            // (PLUS int32, uint32 monoids)_PAIR semirings
            uint64_t t = ((uint64_t) cij) + ainz ;
            cij = (GB_C_TYPE) (t & 0xFFFFFFFFL) ;
        }
        #elif GB_IS_PLUS_BIG_PAIR_SEMIRING
        { 
            // (PLUS int64, uint64, float, or double)_PAIR semirings
            cij += (GB_C_TYPE) ainz ;
        }
        #elif GB_IS_PLUS_FC32_PAIR_SEMIRING
        { 
            // (PLUS monoid for float complex)_PAIR semiring
            cij = GB_CMPLX32 (crealf (cij) + (float) ainz, 0) ;
        }
        #elif GB_IS_PLUS_FC64_PAIR_SEMIRING
        { 
            // (PLUS monoid for double complex)_PAIR semiring
            cij = GB_CMPLX64 (creal (cij) + (double) ainz, 0) ;
        }
        #elif GB_IS_MIN_FIRSTJ_SEMIRING
        {
            // MIN_FIRSTJ semiring: take the 1st entry in A(:,i)
            if (ainz > 0)
            { 
                int64_t k = Ai [pA] + GB_OFFSET ;
                cij = GB_IMIN (cij, k) ;
            }
        }
        #elif GB_IS_MAX_FIRSTJ_SEMIRING
        {
            // MAX_FIRSTJ semiring: take last entry in A(:,i)
            if (ainz > 0)
            { 
                int64_t k = Ai [pA_end-1] + GB_OFFSET ;
                cij = GB_IMAX (cij, k) ;
            }
        }
        #else
        {
            GB_PRAGMA_SIMD_DOT (cij)
            for (int64_t p = pA ; p < pA_end ; p++)
            { 
                int64_t k = Ai [p] ;
                GB_DOT (k, p, pB+k) ;   // cij += A(k,i)*B(k,j)
            }
        }
        #endif

    }
    #else
    {

        //----------------------------------------------------------------------
        // A is sparse/hyper and B is bitmap
        //----------------------------------------------------------------------

        #if GB_IS_MIN_FIRSTJ_SEMIRING
        {
            // MIN_FIRSTJ semiring: take the first entry
            for (int64_t p = pA ; p < pA_end ; p++)
            {
                int64_t k = Ai [p] ;
                if (Bb [pB+k])
                { 
                    cij = GB_IMIN (cij, k + GB_OFFSET) ;
                    break ;
                }
            }
        }
        #elif GB_IS_MAX_FIRSTJ_SEMIRING
        {
            // MAX_FIRSTJ semiring: take the last entry
            for (int64_t p = pA_end-1 ; p >= pA ; p--)
            {
                int64_t k = Ai [p] ;
                if (Bb [pB+k])
                { 
                    cij = GB_IMAX (cij, k + GB_OFFSET) ;
                    break ;
                }
            }
        }
        #else
        {
            GB_PRAGMA_SIMD_DOT (cij)
            for (int64_t p = pA ; p < pA_end ; p++)
            {
                int64_t k = Ai [p] ;
                if (Bb [pB+k])
                { 
                    GB_DOT (k, p, pB+k) ; // cij+=A(k,i)*B(k,j)
                }
            }

        }
        #endif

    }
    #endif

    //--------------------------------------------------------------------------
    // save C(i,j)
    //--------------------------------------------------------------------------

    Cx [pC] = cij ;
}

