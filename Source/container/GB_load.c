//------------------------------------------------------------------------------
// GB_load: load a GrB_Matrix from a Container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_container.h"
#define GB_FREE_ALL \
    GB_phybix_free (A) ;

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
    // printf ("container format %d\n", Container->format) ;

    switch (Container->format)
    {
        case GxB_HYPERSPARSE : 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->h) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->h->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            // fall through to sparse case

        case GxB_SPARSE : 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->p) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->p->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            if (nvals > 0)
            { 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->i) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->i->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            }
            break ;

        case GxB_FULL : 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            if (nrows > 0 && ncols > 0)
            { 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            }
            break ;

        case GxB_BITMAP : 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->b) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            GB_RETURN_IF_NULL (Container->b->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            if (nrows > 0 && ncols > 0)
            { 
                GB_RETURN_IF_NULL (Container->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
                GB_RETURN_IF_NULL (Container->x->x) ;
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            }
            break ;

        default : 
    // printf ("here %s %d\n", __FILE__, __LINE__) ;
            return (GrB_INVALID_VALUE) ;
            break ;
    }
    // printf ("here %s %d\n", __FILE__, __LINE__) ;

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
    A->nvec = Container->nhyper ;   // FIXME
    A->nvec_nonempty = (A->is_csc) ?
        Container->ncols_nonempty : Container->nrows_nonempty ;
    A->iso = Container->iso ;
    A->jumbled = Container->jumbled ;
    uint64_t plen1 = 0, plen, nx ;
    GrB_Type Ap_type = NULL, Ah_type = NULL, Ab_type = NULL, Ai_type = NULL ;
    uint64_t Ah_size = 0, Ap_size = 0, Ai_size = 0, Ab_size = 0, Ax_size = 0 ;

    switch (Container->format)
    {

        case GxB_HYPERSPARSE : 

            A->Y = Container->Y ;
            Container->Y = NULL ;

            GB_OK (GB_vector_unload (Container->h, &(A->h), &plen,
                &Ah_size, &Ah_type, &(A->h_shallow), Werk)) ;
            A->h_size = (size_t) Ah_size ;

            // fall through to the sparse case

        case GxB_SPARSE : 

            GB_OK (GB_vector_unload (Container->p, &(A->p), &plen1,
                &Ap_size, &Ap_type, &(A->p_shallow), Werk)) ;
            printf ("plen1: %lu\n", plen1) ;
            A->p_size = (size_t) Ap_size ;

            GB_OK (GB_vector_unload (Container->i, &(A->i), &nvals,
                &Ai_size, &Ai_type, &(A->i_shallow), Werk)) ;
            A->i_size = (size_t) Ai_size ;

            break ;

        case GxB_BITMAP : 

            GB_OK (GB_vector_unload (Container->b, (void **) &(A->b), &nx,
                &Ab_size, &Ab_type, &(A->b_shallow), Werk)) ;
            A->b_size = (size_t) Ab_size ;
            break ;

        case GxB_FULL : 

        default :;
            break ;
    }

    GB_OK (GB_vector_unload (Container->x, &(A->x), &nx,
        &Ax_size, &(A->type), &(A->x_shallow), Werk)) ;
    A->x_size = (size_t) Ax_size ;

    //--------------------------------------------------------------------------
    // change the type and dimensions of A to match content from Container
    //--------------------------------------------------------------------------

    // A->user_name and all controls are preserved.  Everything else is
    // revised.

    A->plen = plen1 - 1 ;
    A->p_is_32 = (Ap_type == GrB_UINT32) ;
    A->j_is_32 = (Ah_type == GrB_UINT32) ;
    A->i_is_32 = (Ai_type == GrB_UINT32) ;
    A->magic = GB_MAGIC ;

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (A, "A loaded from Container", GB2) ;
    ASSERT_VECTOR_OK (Container->p, "Container->p after load", GB0) ;
    ASSERT_VECTOR_OK (Container->h, "Container->h after load", GB0) ;
    ASSERT_VECTOR_OK (Container->b, "Container->b after load", GB0) ;
    ASSERT_VECTOR_OK (Container->i, "Container->i after load", GB0) ;
    ASSERT_VECTOR_OK (Container->x, "Container->x after load", GB0) ;
    return (GrB_SUCCESS) ;
}

