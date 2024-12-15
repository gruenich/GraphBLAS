//------------------------------------------------------------------------------
// GB_concat_bitmap_bitmap: concatenate a bitmap tile into a bitmap matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

{
    const int A_nthreads = GB_nthreads (anz, chunk, nthreads_max) ;
    const int8_t *restrict Ab = A->b ;
    int64_t pA ;
    #pragma omp parallel for num_threads(A_nthreads) schedule(static)
    for (pA = 0 ; pA < anz ; pA++)
    {
        if (Ab [pA])
        { 
            int64_t i = pA % avlen ;
            int64_t j = pA / avlen ;
            int64_t iC = cistart + i ;
            int64_t jC = cvstart + j ;
            int64_t pC = iC + jC * cvlen ;
            // Cx [pC] = Ax [pA] ;
            GB_COPY (pC, pA, A_iso) ;
            Cb [pC] = 1 ;
        }
    }
}

