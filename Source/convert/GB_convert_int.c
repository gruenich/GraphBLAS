//------------------------------------------------------------------------------
// GB_convert_int: convert the integers in a matrix to/from 32/64 bits
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

// The integer arrays A->[phi] and A->Y in the matrix A are converted to match
// the request p_is_32_new and i_is_32_new.  If converted, A->[phi] are no
// longer shallow.  If A->Y is entirely shallow, it is simply removed from A.
// If A->Y is itself not shallow but contains any shallow A->Y->[phi]
// components, those components are converted and are no longer shallow.

// If A has too many entries for p_is_32_new == true, A->p is left unchanged.
// If the dimension of A is too large for i_is_32_new == true, A->[hi] and A->Y
// are left unchanged.  This is not an error condition.

#include "GB.h"
#define GB_FREE_ALL ;

GrB_Info GB_convert_int     // convert the integers of a matrix
(
    GrB_Matrix A,           // matrix to convert
    bool p_is_32_new,       // new integer format for A->p
    bool i_is_32_new        // new integer format for A->h, A->i, and A->Y
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_MATRIX_OK (A, "A converting integers", GB0) ;
    ASSERT (GB_ZOMBIES_OK (A)) ;
    ASSERT (GB_JUMBLED_OK (A)) ;
    ASSERT (GB_PENDING_OK (A)) ;

    double tt = GB_OPENMP_GET_WTIME ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    if (GB_IS_FULL (A) || GB_IS_BITMAP (A))
    { 
        // quick return: nothing to do
        return (GrB_SUCCESS) ;
    }

    bool p_is_32 = A->p_is_32 ;
    bool i_is_32 = A->i_is_32 ;
    int64_t anz = GB_nnz (A) ;
    int64_t vlen = A->vlen ;
    int64_t vdim = A->vdim ;

    p_is_32_new = GB_validate_p_is_32 (p_is_32_new, anz) ;
    i_is_32_new = GB_validate_i_is_32 (i_is_32_new, vlen, vdim) ;

    if (p_is_32 == p_is_32_new && i_is_32 == i_is_32_new)
    { 
        // quick return: nothing to do
        return (GrB_SUCCESS) ;
    }

    //--------------------------------------------------------------------------
    // at least some integers must be converted
    //--------------------------------------------------------------------------

    // simply remove A->Y if it is entirely shallow
    if (A->Y_shallow)
    { 
        A->Y = NULL ;
        A->Y_shallow = false ;
    }

    bool A_is_hyper = GB_IS_HYPERSPARSE (A) ;
    int64_t plen = A->plen ;
    GrB_Matrix Y = A->Y ;
    GB_Pending Pending = A->Pending ;
    int64_t ynz = GB_nnz (Y) ;
    int64_t yplen = (Y == NULL) ? 0 : Y->plen ;
    int64_t npending = (Pending == NULL) ? 0 : Pending->n ;
    int64_t nmax_pending = (Pending == NULL) ? 0 : Pending->nmax ;

    //--------------------------------------------------------------------------
    // allocate new space for A->[phi] and Y->[pix] if present
    //--------------------------------------------------------------------------

    // Y is not converted via a recurisive call to this method.  Instead, it is
    // converted directly below.  This is because Y->x must also be converted,
    // and also so that the conversion will be all-or-nothing, if out of
    // memory.

    void *Ap_new = NULL ; size_t Ap_new_size = 0 ;
    void *Ah_new = NULL ; size_t Ah_new_size = 0 ;
    void *Ai_new = NULL ; size_t Ai_new_size = 0 ;
    void *Yp_new = NULL ; size_t Yp_new_size = 0 ;
    void *Yi_new = NULL ; size_t Yi_new_size = 0 ;
    void *Yx_new = NULL ; size_t Yx_new_size = 0 ;
    void *Pending_i_new = NULL ; size_t Pending_i_new_size = 0 ;
    void *Pending_j_new = NULL ; size_t Pending_j_new_size = 0 ;
    bool has_Pending_i = (Pending != NULL) && (Pending->i != NULL) ;
    bool has_Pending_j = (Pending != NULL) && (Pending->j != NULL) ;

    size_t psize_new = p_is_32_new ? sizeof (uint32_t) : sizeof (uint64_t) ;
    size_t isize_new = i_is_32_new ? sizeof (uint32_t) : sizeof (uint64_t) ;

    bool ok = true ;

    if (p_is_32 != p_is_32_new)
    { 
        // allocate new space for A->p
        Ap_new = GB_malloc_memory (plen+1, psize_new, &Ap_new_size) ;
        ok = ok && (Ap_new != NULL) ;
    }

    if (i_is_32 != i_is_32_new)
    { 
        // allocate new space for A->i
        Ai_new = GB_malloc_memory (anz, isize_new, &Ai_new_size) ;
        ok = ok && (Ai_new != NULL) ;
        if (A_is_hyper)
        { 
            // allocate new space for A->h
            Ah_new = GB_malloc_memory (plen, isize_new, &Ah_new_size) ;
            ok = ok && (Ah_new != NULL) ;
        }
        if (Y != NULL)
        { 
            // allocate new space for Y->[phi]; matches A->i_is_32
            Yp_new = GB_malloc_memory (yplen+1, isize_new, &Yp_new_size) ;
            Yi_new = GB_malloc_memory (ynz,     isize_new, &Yi_new_size) ;
            Yx_new = GB_malloc_memory (ynz,     isize_new, &Yx_new_size) ;
            ok = ok && (Yp_new != NULL && Yi_new != NULL && Yx_new != NULL) ;
        }
        if (has_Pending_i)
        {
            // allocate new space for Pending->i; matches A->i_is_32
            Pending_i_new = GB_malloc_memory (nmax_pending, isize_new,
                &Pending_i_new_size) ;
            ok = ok && (Pending_i_new != NULL) ;
        }
        if (has_Pending_j)
        {
            // allocate new space for Pending->j; matches A->i_is_32
            Pending_j_new = GB_malloc_memory (nmax_pending, isize_new,
                &Pending_j_new_size) ;
            ok = ok && (Pending_j_new != NULL) ;
        }
    }

    if (!ok)
    { 
        // out of memory: A is unchanged
        GB_FREE (&Ap_new, Ap_new_size) ;
        GB_FREE (&Ah_new, Ah_new_size) ;
        GB_FREE (&Ai_new, Ai_new_size) ;
        GB_FREE (&Yp_new, Yp_new_size) ;
        GB_FREE (&Yi_new, Yi_new_size) ;
        GB_FREE (&Yx_new, Yx_new_size) ;
        GB_FREE (&Pending_i_new, Pending_i_new_size) ;
        GB_FREE (&Pending_j_new, Pending_j_new_size) ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    // the conversion will now succeed

    //--------------------------------------------------------------------------
    // convert A->p
    //--------------------------------------------------------------------------

    int nthreads_max = GB_Context_nthreads_max ( ) ;

    if (p_is_32 != p_is_32_new)
    { 
        GB_cast_int (Ap_new, p_is_32_new ? GB_UINT32_code : GB_UINT64_code,
                     A->p  , p_is_32     ? GB_UINT32_code : GB_UINT64_code,
                     plen+1, nthreads_max) ;
        if (!A->p_shallow)
        { 
            GB_FREE (&(A->p), A->p_size) ;
        }
        A->p = Ap_new ;
        A->p_size = Ap_new_size ;
        A->p_shallow = false ;
        A->p_is_32 = p_is_32_new ;
    }

    //--------------------------------------------------------------------------
    // convert A->h, A->i, Y->p, Y->i, Pending->i, and Pending->j
    //--------------------------------------------------------------------------

    if (i_is_32 != i_is_32_new)
    { 

        size_t icode_new = i_is_32_new ? GB_INT32_code  : GB_INT64_code  ;
        size_t icode     = i_is_32     ? GB_INT32_code  : GB_INT64_code  ;
        size_t ucode_new = i_is_32_new ? GB_UINT32_code : GB_UINT64_code ;
        size_t ucode     = i_is_32     ? GB_UINT32_code : GB_UINT64_code ;

        //----------------------------------------------------------------------
        // convert A->i
        //----------------------------------------------------------------------

        GB_cast_int (Ai_new, icode_new, A->i, icode, anz, nthreads_max) ;
        if (!A->i_shallow)
        { 
            GB_FREE (&(A->i), A->i_size) ;
        }
        A->i = Ai_new ;
        A->i_size = Ai_new_size ;
        A->i_shallow = false ;
        A->i_is_32 = i_is_32_new ;

        //----------------------------------------------------------------------
        // convert A->h if present
        //----------------------------------------------------------------------

        if (A_is_hyper)
        { 
            GB_cast_int (Ah_new, ucode_new, A->h, ucode, plen, nthreads_max) ;
            if (!A->h_shallow)
            { 
                GB_FREE (&(A->h), A->h_size) ;
            }
            A->h = Ah_new ;
            A->h_size = Ah_new_size ;
            A->h_shallow = false ;
        }

        //----------------------------------------------------------------------
        // convert A->Y if present
        //----------------------------------------------------------------------

        if (Y != NULL)
        { 
            // A is hypersparse, and the integers of Y match A->i_is_32
            ASSERT (A_is_hyper) ;
            ASSERT (Y->p_is_32 == i_is_32) ;
            ASSERT (Y->i_is_32 == i_is_32) ;

            //------------------------------------------------------------------
            // convert Y->p
            //------------------------------------------------------------------

            GB_cast_int (Yp_new, ucode_new, Y->p, ucode, yplen+1, nthreads_max);
            if (!Y->p_shallow)
            { 
                GB_FREE (&(Y->p), Y->p_size) ;
            }
            Y->p = Ap_new ;
            Y->p_size = Ap_new_size ;
            Y->p_shallow = false ;
            Y->p_is_32 = i_is_32_new ;

            //------------------------------------------------------------------
            // convert Y->i
            //------------------------------------------------------------------

            GB_cast_int (Yi_new, icode_new, Y->i, icode, ynz, nthreads_max) ;
            if (!Y->i_shallow)
            { 
                GB_FREE (&(Y->i), Y->i_size) ;
            }
            Y->i = Yi_new ;
            Y->i_size = Yi_new_size ;
            Y->i_shallow = false ;
            Y->i_is_32 = i_is_32_new ;

            //------------------------------------------------------------------
            // convert Y->x
            //------------------------------------------------------------------

            GB_cast_int (Yx_new, ucode_new, Y->x, ucode, ynz, nthreads_max) ;
            if (!Y->x_shallow)
            { 
                GB_FREE (&(Y->x), Y->x_size) ;
            }
            Y->x = Yx_new ;
            Y->x_size = Yx_new_size ;
            Y->x_shallow = false ;
            Y->type = i_is_32_new ? GrB_UINT32 : GrB_UINT64 ;
        }

        //----------------------------------------------------------------------
        // convert Pending->i and Pending->j if present
        //----------------------------------------------------------------------

        if (has_Pending_i)
        { 

            //------------------------------------------------------------------
            // convert Pending->i
            //------------------------------------------------------------------

            GB_cast_int (Pending_i_new, ucode_new, Pending->i, ucode, npending,
                nthreads_max) ;
            GB_FREE (&(Pending->i), Pending->i_size) ;
            Pending->i = Pending_i_new ;
            Pending->i_size = Pending_i_new_size ;
        }

        if (has_Pending_j)
        {

            //------------------------------------------------------------------
            // convert Pending->j
            //------------------------------------------------------------------

            GB_cast_int (Pending_j_new, ucode_new, Pending->j, ucode, npending,
                nthreads_max) ;
            GB_FREE (&(Pending->j), Pending->j_size) ;
            Pending->j = Pending_j_new ;
            Pending->j_size = Pending_j_new_size ;
        }
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    tt = GB_OPENMP_GET_WTIME - tt;
    GB_BURBLE_MATRIX (A, "(convert ints %s/%s to %s/%s, time: %g) ",
        p_is_32 ? "32" : "64",
        i_is_32 ? "32" : "64",
        p_is_32_new ? "32" : "64",
        i_is_32_new ? "32" : "64", tt) ;
    ASSERT_MATRIX_OK (A, "A integers converted", GB0) ;
    return (GrB_SUCCESS) ;
}

