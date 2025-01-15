//------------------------------------------------------------------------------
// GB_unload: unload a GrB_Matrix into a Container
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
#define GB_DEBUG

// DONE: 32/64 bit

// This method takes O(1) time and performs no mallocs at all, unless A has
// pending work that must be finished.  It typically will perform no frees,
// unless A has an error string in A->logger, or if the Container has prior
// content (which is freed).

#include "GB_container.h"
#define GB_FREE_ALL ;

GrB_Info GB_unload              // GrB_Matrix -> GxB_Container
(
    GrB_Matrix A,               // matrix to unload into the Container
    GxB_Container Container     // Container to hold the contents of A
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_RETURN_IF_NULL_OR_FAULTY (A) ;
    GB_RETURN_IF_NULL (Container) ;
    GB_WHERE_1 (A, "GxB_unload_into_Container") ;
    ASSERT_MATRIX_OK (A, "A to unload into Container", GB0) ;
    ASSERT_VECTOR_OK (Container->p, "Container->p before unload", GB0) ;
    ASSERT_VECTOR_OK (Container->h, "Container->h before unload", GB0) ;
    ASSERT_VECTOR_OK (Container->b, "Container->b before unload", GB0) ;
    ASSERT_VECTOR_OK (Container->i, "Container->i before unload", GB0) ;
    ASSERT_VECTOR_OK (Container->x, "Container->x before unload", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (Container->Y, "Container->Y before unload", GB0) ;

    //--------------------------------------------------------------------------
    // finish any pending work
    //--------------------------------------------------------------------------

    GB_MATRIX_WAIT (A) ;

    //--------------------------------------------------------------------------
    // unload the matrix into the container
    //--------------------------------------------------------------------------

    int64_t nvals = GB_nnz (A) ;
    int64_t nheld = GB_nnz_held (A) ;
    bool iso = A->iso ;

    bool is_csc = A->is_csc ;
    Container->nrows = (is_csc) ? A->vlen : A->vdim ;
    Container->ncols = (is_csc) ? A->vdim : A->vlen ;
    Container->nrows_nonempty = (is_csc) ? -1 : A->nvec_nonempty ;
    Container->ncols_nonempty = (is_csc) ? A->nvec_nonempty : -1 ;
    Container->nvals = nvals ;
    Container->nheld = nheld ;
    Container->nvec = A->nvec ;
    Container->format = GB_sparsity (A) ;
    Container->orientation = (is_csc) ? GrB_COLMAJOR : GrB_ROWMAJOR ;
    Container->iso = iso ;
    Container->jumbled = A->jumbled ;

    switch (Container->format)
    {

        case GxB_HYPERSPARSE : 

            if (Container->Y != NULL)
            { 
                // free any prior content
                GB_Matrix_free (&(Container->Y)) ;
            }

            // unload A->Y into the Container unless it is entirely shallow
            if (!A->Y_shallow)
            { 
                // A->Y may still have shallow components, which is OK
                Container->Y = A->Y ;
                A->Y = NULL ;
            }

            GB_vector_load (Container->h, &(A->h), A->plen, A->h_size,
                A->j_is_32 ? GrB_UINT32 : GrB_UINT64, A->h_shallow) ;

            // fall through to the sparse case

        case GxB_SPARSE : 

            GB_vector_load (Container->p, &(A->p), A->plen+1, A->p_size,
                A->p_is_32 ? GrB_UINT32 : GrB_UINT64, A->p_shallow) ;
            GB_vector_load (Container->i, &(A->i), nvals, A->i_size,
                A->i_is_32 ? GrB_UINT32 : GrB_UINT64, A->i_shallow) ;
            break ;

        case GxB_BITMAP : 

            GB_vector_load (Container->b, (void **) &(A->b), nheld, A->b_size,
                GrB_INT8, A->b_shallow) ;

        case GxB_FULL : 
        default :;
            break ;
    }

    GB_vector_load (Container->x, &(A->x), iso ? 1 : nheld, A->x_size,
        A->type, A->x_shallow) ;

    //--------------------------------------------------------------------------
    // change A to a dense 0-by-0 matrix with no content
    //--------------------------------------------------------------------------

    // A->user_name, A->type, and all controls are preserved.  Everything else
    // is revised.

    GB_phybix_free (A) ;
    A->plen = -1 ;
    A->vlen = 0 ;
    A->vdim = 0 ;
    A->nvec_nonempty = 0 ;
    A->p_is_32 = false ;
    A->j_is_32 = false ;
    A->i_is_32 = false ;
    A->magic = GB_MAGIC ;

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (A, "A unloaded into Container", GB0) ;
    ASSERT_VECTOR_OK (Container->p, "Container->p after unload", GB0) ;
    ASSERT_VECTOR_OK (Container->h, "Container->h after unload", GB0) ;
    ASSERT_VECTOR_OK (Container->b, "Container->b after unload", GB0) ;
    ASSERT_VECTOR_OK (Container->i, "Container->i after unload", GB0) ;
    ASSERT_VECTOR_OK (Container->x, "Container->x after unload", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (Container->Y, "Container->Y after unload", GB0) ;
    return (GrB_SUCCESS) ;
}

