//------------------------------------------------------------------------------
// GB_resize: change the size of a matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#define GB_DEBUG

// FIXME: 32/64 bit

#include "select/GB_select.h"
#include "scalar/GB_Scalar_wrap.h"
#include "resize/GB_resize.h"

#define GB_FREE_ALL                     \
{                                       \
    GB_Matrix_free (&T) ;               \
    GB_FREE (&Ax_new, Ax_new_size) ;    \
    GB_FREE (&Ab_new, Ab_new_size) ;    \
    GB_phybix_free (A) ;                \
}

//------------------------------------------------------------------------------
// GB_resize: resize a GrB_Matrix
//------------------------------------------------------------------------------

GrB_Info GB_resize              // change the size of a matrix
(
    GrB_Matrix A,               // matrix to modify
    const GrB_Index nrows_new,  // new number of rows in matrix
    const GrB_Index ncols_new,  // new number of columns in matrix
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    GB_void *restrict Ax_new = NULL ; size_t Ax_new_size = 0 ;
    int8_t  *restrict Ab_new = NULL ; size_t Ab_new_size = 0 ;
    ASSERT_MATRIX_OK (A, "A to resize", GB0) ;

    struct GB_Matrix_opaque T_header ;
    GrB_Matrix T = NULL ;

    //--------------------------------------------------------------------------
    // handle the CSR/CSC format
    //--------------------------------------------------------------------------

    int64_t vdim_old = A->vdim ;
    int64_t vlen_old = A->vlen ;
    int64_t vlen_new, vdim_new ;
    if (A->is_csc)
    { 
        vlen_new = nrows_new ;
        vdim_new = ncols_new ;
    }
    else
    { 
        vlen_new = ncols_new ;
        vdim_new = nrows_new ;
    }

    if (vdim_new == vdim_old && vlen_new == vlen_old)
    { 
        // nothing to do
        return (GrB_SUCCESS) ;
    }

    //--------------------------------------------------------------------------
    // delete any lingering zombies and assemble any pending tuples
    //--------------------------------------------------------------------------

    // only do so if either dimension is shrinking, or if pending tuples exist
    // and vdim_old <= 1 and vdim_new > 1, since in that case, Pending->j has
    // not been allocated yet, but would be required in the resized matrix.
    // If A is jumbled, it must be sorted.

    // FIXME: if change in max(vlen,vdim) will cause A->i_is_32 to change, then
    // assemble pending tuples here too

    if (vdim_new < vdim_old || vlen_new < vlen_old || A->jumbled ||
        (GB_PENDING (A) && vdim_old <= 1 && vdim_new > 1))
    { 
        GB_MATRIX_WAIT (A) ;
        ASSERT_MATRIX_OK (A, "A to resize, wait", GB0) ;
    }

    ASSERT (!GB_JUMBLED (A)) ;
    ASSERT_MATRIX_OK (A, "Final A to resize", GB0) ;

    //--------------------------------------------------------------------------
    // resize the matrix
    //--------------------------------------------------------------------------

    const bool A_is_bitmap = GB_IS_BITMAP (A) ;
    const bool A_is_full = GB_IS_FULL (A) ;
    const bool A_is_shrinking = (vdim_new <= vdim_old && vlen_new <= vlen_old) ;

    if ((A_is_full || A_is_bitmap) && A_is_shrinking)
    {

        //----------------------------------------------------------------------
        // A is full or bitmap
        //----------------------------------------------------------------------

        // get the old and new dimensions
        int64_t anz_new = 1 ;
        bool ok = GB_int64_multiply ((GrB_Index *) (&anz_new),
            vlen_new, vdim_new) ;
        if (!ok) anz_new = 1 ;
        size_t nzmax_new = GB_IMAX (anz_new, 1) ;
        bool in_place = A_is_full && (vlen_new == vlen_old || vdim_new <= 1) ;
        size_t asize = A->type->size ;
        const bool A_iso = A->iso ;

        //----------------------------------------------------------------------
        // allocate or reallocate A->x, unless A is iso
        //----------------------------------------------------------------------

        ok = true ;
        if (!A_iso)
        {
            if (in_place)
            { 
                // reallocate A->x in-place; no data movement needed
                GB_REALLOC (A->x, nzmax_new*asize, GB_void, &(A->x_size), &ok) ;
            }
            else
            { 
                // allocate new space for A->x; use calloc to ensure all space
                // is initialized.
                Ax_new = GB_CALLOC (nzmax_new*asize, GB_void, &Ax_new_size) ;
                ok = (Ax_new != NULL) ;
            }
        }

        //----------------------------------------------------------------------
        // allocate or reallocate A->b
        //----------------------------------------------------------------------

        if (!in_place && A_is_bitmap)
        { 
            // allocate new space for A->b
            Ab_new = GB_MALLOC (nzmax_new*asize, int8_t, &Ab_new_size) ;
            ok = ok && (Ab_new != NULL) ;
        }

        if (!ok)
        { 
            // out of memory
            GB_FREE_ALL ;
            return (GrB_OUT_OF_MEMORY) ;
        }

        //----------------------------------------------------------------------
        // move data if not in-place
        //----------------------------------------------------------------------

        if (!in_place)
        {

            //------------------------------------------------------------------
            // determine number of threads to use
            //------------------------------------------------------------------

            int nthreads_max = GB_Context_nthreads_max ( ) ;
            double chunk = GB_Context_chunk ( ) ;
            int nthreads = GB_nthreads (anz_new, chunk, nthreads_max) ;

            //------------------------------------------------------------------
            // resize Ax, unless A is iso
            //------------------------------------------------------------------
        
            if (!A_iso)
            {
                GB_void *restrict Ax_old = (GB_void *) A->x ;

                int64_t j ;
                if (vdim_new <= 4*nthreads)
                {
                    // use all threads for each vector
                    for (j = 0 ; j < vdim_new ; j++)
                    { 
                        GB_void *pdest = Ax_new + j * vlen_new * asize ;
                        GB_void *psrc  = Ax_old + j * vlen_old * asize ;
                        GB_memcpy (pdest, psrc, vlen_new * asize, nthreads) ;
                    }
                }
                else
                {
                    // use a single thread for each vector
                    #pragma omp parallel for num_threads(nthreads) \
                        schedule(static)
                    for (j = 0 ; j < vdim_new ; j++)
                    { 
                        GB_void *pdest = Ax_new + j * vlen_new * asize ;
                        GB_void *psrc  = Ax_old + j * vlen_old * asize ;
                        memcpy (pdest, psrc, vlen_new * asize) ;
                    }
                }
                GB_FREE (&Ax_old, A->x_size) ;
                A->x = Ax_new ; A->x_size = Ax_new_size ;
            }

            //------------------------------------------------------------------
            // resize Ab if A is bitmap, and count the # of entries
            //------------------------------------------------------------------

            if (A_is_bitmap)
            {
                int8_t *restrict Ab_old = A->b ;
                int64_t pnew ;
                int64_t anvals = 0 ;
                #pragma omp parallel for num_threads(nthreads) \
                    schedule(static) reduction(+:anvals)
                for (pnew = 0 ; pnew < anz_new ; pnew++)
                { 
                    int64_t i = pnew % vlen_new ;
                    int64_t j = pnew / vlen_new ;
                    int64_t pold = i + j * vlen_old ;
                    int8_t ab = Ab_old [pold] ;
                    Ab_new [pnew] = ab ;
                    anvals += ab ;
                }
                A->nvals = anvals ;
                GB_FREE (&Ab_old, A->b_size) ;
                A->b = Ab_new ; A->b_size = Ab_new_size ;
            }
        }

        //----------------------------------------------------------------------
        // adjust dimensions and return result
        //----------------------------------------------------------------------

        A->vdim = vdim_new ;
        A->vlen = vlen_new ;
        A->nvec = vdim_new ;
        A->nvec_nonempty = (vlen_new == 0) ? 0 : vdim_new ;

    }
    else
    {

        //----------------------------------------------------------------------
        // convert A to hypersparse and resize it
        //----------------------------------------------------------------------

        // convert to hypersparse
        GB_OK (GB_convert_any_to_hyper (A, Werk)) ;
        ASSERT (GB_IS_HYPERSPARSE (A)) ;
        ASSERT_MATRIX_OK (A, "A converted to hyper", GB0) ;

        // A->Y will be invalidated, so free it
        GB_hyper_hash_free (A) ;

        // resize the number of sparse vectors
        uint64_t *restrict Ap = A->p ;  // FIXME
        int64_t *restrict Ah = A->h ;   // FIXME

        if (vdim_new < vdim_old)
        { 
            // descrease A->nvec to delete the vectors outside the range
            // 0...vdim_new-1.
            int64_t pleft = 0 ;
            int64_t pright = GB_IMIN (A->nvec, vdim_new) - 1 ;
            bool found ;
            GB_SPLIT_BINARY_SEARCH (vdim_new, Ah, pleft, pright, found) ;//FIXME
            A->nvec = pleft ;
            A->nvals = Ap [A->nvec] ;   // FIXME

            // number of vectors is decreasing, need to count the new number of
            // non-empty vectors: done during pruning or by selector, below.
            A->nvec_nonempty = -1 ;     // recomputed just below
        }

        if (vdim_new < A->plen)
        { 
            // reduce the size of A->p and A->h; this cannot fail
            info = GB_hyper_realloc (A, vdim_new, Werk) ;
            ASSERT (info == GrB_SUCCESS) ;
        }

        A->vdim = vdim_new ;
        ASSERT_MATRIX_OK (A, "A vdim resized", GB0) ;

        //----------------------------------------------------------------------
        // resize the length of each vector
        //----------------------------------------------------------------------

        // if vlen is shrinking, delete entries outside the new matrix
        if (vlen_new < vlen_old)
        { 
            // A = select (A), keeping entries in rows <= vlen_new-1
            struct GB_Scalar_opaque Thunk_header ;
            int64_t k = vlen_new - 1 ;
            GrB_Scalar Thunk = GB_Scalar_wrap (&Thunk_header, GrB_INT64, &k) ;
            GB_CLEAR_STATIC_HEADER (T, &T_header) ;
            GB_OK (GB_selector (T, GrB_ROWLE, false, A, Thunk, Werk)) ;
            GB_OK (GB_transplant (A, A->type, &T, false, Werk)) ;
            ASSERT_MATRIX_OK (A, "A rows pruned", GB0) ;
        }

        //----------------------------------------------------------------------
        // vlen has been resized
        //----------------------------------------------------------------------

        A->vlen = vlen_new ;

        // FIXME: change A->p_is_32 and A->i_is_32 here, and do GB_convert_int
        #if 0
        if A->p_is_32 or i_is_32 are changing then
            GB_convert_int ( ... )   FIXME
        #endif

        ASSERT_MATRIX_OK (A, "A vlen resized", GB0) ;

        //----------------------------------------------------------------------
        // conform the matrix to its desired sparsity structure
        //----------------------------------------------------------------------

        GB_OK (GB_conform (A, Werk)) ;  // FIXME: move below
        ASSERT (A->nvec_nonempty >= 0) ;
    }

    // FIXME: conform here
    ASSERT_MATRIX_OK (A, "A final resized", GB0) ;
    return (GrB_SUCCESS) ;
}

