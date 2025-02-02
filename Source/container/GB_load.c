//------------------------------------------------------------------------------
// GB_load: load a GrB_Matrix from a Container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// A->user_name and all controls are preserved.  Everything else in the matrix
// A is revised: the dimensions, type, content, 32/64 integer status, iso
// status, jumbled status, orientation (by row/col), etc.

#include "GB_container.h"
#define GB_FREE_ALL ;

#define CHECK(condition)                \
    if (!(condition))                   \
    {                                   \
        return (GrB_INVALID_VALUE) ;    \
    }

GrB_Info GB_load                // GxB_Container -> GrB_Matrix
(
    GrB_Matrix A,               // matrix to load from the Container
    GxB_Container Container,    // Container with contents to load into A
    GB_Werk Werk
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_MATRIX_OK (A, "A to load from Container", GB0) ;
    ASSERT_VECTOR_OK (Container->p, "Container->p before load", GB0) ;
    ASSERT_VECTOR_OK (Container->h, "Container->h before load", GB0) ;
    ASSERT_VECTOR_OK (Container->b, "Container->b before load", GB0) ;
    ASSERT_VECTOR_OK (Container->i, "Container->i before load", GB0) ;
    ASSERT_VECTOR_OK (Container->x, "Container->x before load", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (Container->Y, "Container->Y before load", GB0) ;

    //--------------------------------------------------------------------------
    // quick sanity checks
    //--------------------------------------------------------------------------

    uint64_t nvals = Container->nvals ;
    uint64_t nrows = Container->nrows ;
    uint64_t ncols = Container->ncols ;

    switch (Container->format)
    {
        case GxB_HYPERSPARSE : 
            GB_RETURN_IF_NULL (Container->h) ;
            GB_RETURN_IF_NULL (Container->h->x) ;
            // fall through to sparse case

        case GxB_SPARSE : 
            GB_RETURN_IF_NULL (Container->p) ;
            GB_RETURN_IF_NULL (Container->p->x) ;
            if (nvals > 0)
            { 
                GB_RETURN_IF_NULL (Container->i) ;
                GB_RETURN_IF_NULL (Container->i->x) ;
                GB_RETURN_IF_NULL (Container->x) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
            }
            break ;

        case GxB_BITMAP : 
            GB_RETURN_IF_NULL (Container->b) ;
            GB_RETURN_IF_NULL (Container->b->x) ;
            if (nrows > 0 && ncols > 0)
            { 
                GB_RETURN_IF_NULL (Container->x) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
            }
            break ;

        case GxB_FULL : 
            if (nrows > 0 && ncols > 0)
            { 
                GB_RETURN_IF_NULL (Container->x) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
            }
            nvals = 0 ; // not used when A is full; see GB_nnz.
            break ;

        default : 
            return (GrB_INVALID_VALUE) ;
            break ;
    }

    //--------------------------------------------------------------------------
    // free any prior content of A
    //--------------------------------------------------------------------------

    GB_phybix_free (A) ;

    //--------------------------------------------------------------------------
    // load the matrix from the container
    //--------------------------------------------------------------------------

    A->nvals = nvals ;
    A->is_csc = (Container->orientation == GrB_COLMAJOR) ;
    A->vlen = (A->is_csc) ? nrows : ncols ;
    A->vdim = (A->is_csc) ? ncols : nrows ;
    A->nvec = Container->nhyper ;   // FIXME remove this
    // FIXME: set A->nvec to A->vdim for bitmap/full,
    //  to A->plen for sparse
    //  to length of A->h for hypersparse

    A->nvec_nonempty = (A->is_csc) ?
        Container->ncols_nonempty : Container->nrows_nonempty ;
    A->iso = Container->iso ;
    A->jumbled = Container->jumbled ;
    uint64_t plen1 = 0, plen, nx ;
    GrB_Type Ap_type = NULL, Ah_type = NULL, Ab_type = NULL, Ai_type = NULL ;
    uint64_t Ah_size = 0, Ap_size = 0, Ai_size = 0, Ab_size = 0, Ax_size = 0 ;

    // get or clear the phybix content: Ap, Ah, A->Y, A->b, A->i, and A->x,
    // depending on the format of the date held in the container

    switch (Container->format)
    {

        case GxB_HYPERSPARSE : 

            // load A->p, A->h, A->Y, and A->i from the container
            GB_OK (GB_vector_unload (Container->p, &(A->p), &plen1,
                &Ap_size, &Ap_type, &(A->p_shallow), Werk)) ;
            A->p_size = (size_t) Ap_size ;
            GB_OK (GB_vector_unload (Container->h, &(A->h), &plen,
                &Ah_size, &Ah_type, &(A->h_shallow), Werk)) ;
            A->h_size = (size_t) Ah_size ;
            A->Y = Container->Y ;
            Container->Y = NULL ;
            GB_vector_reset (Container->b) ;
            GB_OK (GB_vector_unload (Container->i, &(A->i), &nvals,
                &Ai_size, &Ai_type, &(A->i_shallow), Werk)) ;
            A->i_size = (size_t) Ai_size ;
            // FIXME: ensure length of A->p is nvec+1
            A->plen = plen ;
            CHECK (plen1 == plen + 1) ;
            break ;

        case GxB_SPARSE : 

            // load A->p and A->i from the container
            GB_OK (GB_vector_unload (Container->p, &(A->p), &plen1,
                &Ap_size, &Ap_type, &(A->p_shallow), Werk)) ;
            A->p_size = (size_t) Ap_size ;
            GB_vector_reset (Container->h) ;
            GB_Matrix_free (&(Container->Y)) ;
            GB_vector_reset (Container->b) ;
            GB_OK (GB_vector_unload (Container->i, &(A->i), &nvals,
                &Ai_size, &Ai_type, &(A->i_shallow), Werk)) ;
            A->i_size = (size_t) Ai_size ;
            // FIXME: ensure length of A->p is vdim+1
            A->plen = plen1 - 1 ;
            break ;

        case GxB_BITMAP : 

            // load A->b from the container
            GB_vector_reset (Container->p) ;
            GB_vector_reset (Container->h) ;
            GB_Matrix_free (&(Container->Y)) ;
            GB_OK (GB_vector_unload (Container->b, (void **) &(A->b), &nx,
                &Ab_size, &Ab_type, &(A->b_shallow), Werk)) ;
            A->b_size = (size_t) Ab_size ;
            GB_vector_reset (Container->i) ;
            // FIXME: set A->plen = -1
            // FIXME: ensure nx is nrows*ncols
            A->plen = -1 ;
            break ;

        case GxB_FULL : 

            // clear phybi components
            GB_vector_reset (Container->p) ;
            GB_vector_reset (Container->h) ;
            GB_Matrix_free (&(Container->Y)) ;
            GB_vector_reset (Container->b) ;
            GB_vector_reset (Container->i) ;
            // FIXME: set A->plen = -1
            A->plen = -1 ;
            break ;

        default :;
            break ;
    }

    GB_OK (GB_vector_unload (Container->x, &(A->x), &nx,
        &Ax_size, &(A->type), &(A->x_shallow), Werk)) ;
    A->x_size = (size_t) Ax_size ;

    // FIXME: ensure nx is 1 if iso,
    // A->nvals for the sparse/hypersparse cases, or
    // nrows*ncols if full/bitmap

//  A->plen = plen1 - 1 ;
    A->p_is_32 = (Ap_type == GrB_UINT32) ;
    A->j_is_32 = (Ah_type == GrB_UINT32) ;
    A->i_is_32 = (Ai_type == GrB_UINT32) ;
    A->magic = GB_MAGIC ;

    // FIXME: if sparse or hypersparse, ensure Ap [plen] == nvals here

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (A, "A loaded from Container", GB0) ;
    ASSERT_VECTOR_OK (Container->p, "Container->p after load", GB0) ;
    ASSERT_VECTOR_OK (Container->h, "Container->h after load", GB0) ;
    ASSERT_VECTOR_OK (Container->b, "Container->b after load", GB0) ;
    ASSERT_VECTOR_OK (Container->i, "Container->i after load", GB0) ;
    ASSERT_VECTOR_OK (Container->x, "Container->x after load", GB0) ;
    return (GrB_SUCCESS) ;
}

