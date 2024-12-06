//------------------------------------------------------------------------------
// GB_cumsum_template: cumlative sum of an array (uint32_t or uint64_t)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Compute the cumulative sum of an array count[0:n], of size n+1:

//      k = sum (count [0:n-1] != 0) ;
//      count = cumsum ([0 count[0:n-1]]) ;

// That is, count [j] on input is overwritten with sum (count [0..j-1]).
// On input, count [n] is not accessed and is implicitly zero on input.
// On output, count [n] is the total sum.

{
    if (kresult == NULL)
    {

        if (nthreads <= 2)
        {

            //------------------------------------------------------------------
            // cumsum with one thread
            //------------------------------------------------------------------

            return (GB_CUMSUM (count, n)) ;

        }
        else
        {

            //------------------------------------------------------------------
            // cumsum with multiple threads
            //------------------------------------------------------------------

            // allocate workspace
            GB_WERK_DECLARE (ws, int64_t) ;
            GB_WERK_PUSH (ws, nthreads, int64_t) ;
            if (ws == NULL)
            { 
                // out of memory; use a single thread instead
                return (GB_CUMSUM (count, n)) ;
            }

            int tid ;
            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (tid = 0 ; tid < nthreads ; tid++)
            {
                // each task sums up its own part
                int64_t istart, iend ;
                GB_PARTITION (istart, iend, n, tid, nthreads) ;
                uint64_t s = 0 ;
                for (int64_t i = istart ; i < iend ; i++)
                { 
                    s += count [i] ;
                }
                ws [tid] = s ;
            }

            #if GB_CHECK_OVERFLOW
            {
                uint64_t total = 0 ;
                for (tid = 0 ; tid < nthreads ; tid++)
                { 
                    total += ws [tid] ;
                }
                if (total > UINT32_MAX)
                { 
                    return (false) ;
                }
            }
            #endif

            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (tid = 0 ; tid < nthreads ; tid++)
            {
                // each tasks computes the cumsum of its own part
                int64_t istart, iend ;
                GB_PARTITION (istart, iend, n, tid, nthreads) ;
                uint64_t s = 0 ;
                for (int i = 0 ; i < tid ; i++)
                { 
                    s += ws [i] ;
                }
                for (int64_t i = istart ; i < iend ; i++)
                { 
                    uint64_t c = count [i] ;
                    count [i] = s ;
                    s += c ;
                }
                if (iend == n)
                { 
                    count [n] = s ;
                }
            }

            // free workspace
            GB_WERK_POP (ws, int64_t) ;
        }

    }
    else
    {

        if (nthreads <= 2)
        {

            //------------------------------------------------------------------
            // cumsum with one thread, also compute k
            //------------------------------------------------------------------

            int64_t k = 0 ;
            uint64_t s = 0 ;
            for (int64_t i = 0 ; i < n ; i++)
            { 
                uint64_t c = count [i] ;
                if (c != 0) k++ ;
                count [i] = s ;
                s += c ;
            }
            count [n] = s ;
            (*kresult) = k ;

            #if GB_CHECK_OVERFLOW
            if (s > UINT32_MAX)
            { 
                return (false) ;
            }
            #endif

        }
        else
        {

            //------------------------------------------------------------------
            // cumsum with multiple threads, also compute k
            //------------------------------------------------------------------

            // allocate workspace
            GB_WERK_DECLARE (ws, int64_t) ;
            GB_WERK_DECLARE (wk, int64_t) ;
            GB_WERK_PUSH (ws, nthreads, int64_t) ;
            GB_WERK_PUSH (wk, nthreads, int64_t) ;
            if (ws == NULL || wk == NULL)
            { 
                // out of memory; use a single thread instead
                GB_WERK_POP (wk, int64_t) ;
                GB_WERK_POP (ws, int64_t) ;
                return (GB_cumsum (count, count_is_32, n, kresult, 1, NULL)) ;
            }

            int tid ;
            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (tid = 0 ; tid < nthreads ; tid++)
            {
                // each task sums up its own part
                int64_t istart, iend ;
                GB_PARTITION (istart, iend, n, tid, nthreads) ;
                int64_t k = 0 ;
                int64_t s = 0 ;
                for (int64_t i = istart ; i < iend ; i++)
                { 
                    int64_t c = count [i] ;
                    if (c != 0) k++ ;
                    s += c ;
                }
                ws [tid] = s ;
                wk [tid] = k ;
            }

            #if GB_CHECK_OVERFLOW
            {
                uint64_t total = 0 ;
                for (tid = 0 ; tid < nthreads ; tid++)
                { 
                    total += ws [tid] ;
                }
                if (total > UINT32_MAX)
                { 
                    return (false) ;
                }
            }
            #endif

            #pragma omp parallel for num_threads(nthreads) schedule(static)
            for (tid = 0 ; tid < nthreads ; tid++)
            {
                // each task computes the cumsum of its own part
                int64_t istart, iend ;
                GB_PARTITION (istart, iend, n, tid, nthreads) ;
                int64_t s = 0 ;
                for (int i = 0 ; i < tid ; i++)
                { 
                    s += ws [i] ;
                }
                for (int64_t i = istart ; i < iend ; i++)
                { 
                    int64_t c = count [i] ;
                    count [i] = s ;
                    s += c ;
                }
                if (iend == n)
                { 
                    count [n] = s ;
                }
            }

            int64_t k = 0 ;
            for (int tid = 0 ; tid < nthreads ; tid++)
            { 
                k += wk [tid] ;
            }
            (*kresult) = k ;

            // free workspace
            GB_WERK_POP (wk, int64_t) ;
            GB_WERK_POP (ws, int64_t) ;
        }
    }
}

#undef GB_CHECK_OVERFLOW
#undef GB_CUMSUM

