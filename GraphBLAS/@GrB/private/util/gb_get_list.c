//------------------------------------------------------------------------------
// gb_get_list: return GrB_Vector for assign, subassign, extract, and build
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#define GB_DEBUG

#include "gb_interface.h"

GrB_Vector gb_get_list      // list of indices or values
(
    const mxArray *X,       // MATLAB input matrix or struct with GrB content
    const int base_offset   // 1 or 0
)
{ 

    //--------------------------------------------------------------------------
    // get a shallow GrB_Matrix S of the input MATLAB matrix or struct
    //--------------------------------------------------------------------------

    GrB_Matrix C = NULL ;
    GrB_Vector V = NULL ;
    GrB_Matrix S = gb_get_shallow (X) ;

    //--------------------------------------------------------------------------
    // get the properties of S
    //--------------------------------------------------------------------------

    GrB_Type type ;
    uint64_t ncols, nrows, n ;
    OK (GrB_Matrix_nrows (&nrows, S)) ;
    OK (GrB_Matrix_ncols (&ncols, S)) ;
    OK (GxB_Matrix_type (&type, S)) ;

    //--------------------------------------------------------------------------
    // check for quick return
    //--------------------------------------------------------------------------

    if (ncols == 0 || nrows == 0)
    { 
        // return a zero-length vector
        OK (GrB_Vector_new (&V, type, 0)) ;
        ASSERT_VECTOR_OK (V, "V result, empty", GB0) ;
        return (V) ;
    }

    int sparsity, fmt ;
    OK (GrB_get (S, &fmt, GxB_FORMAT)) ;
    OK (GrB_get (S, &sparsity, GxB_SPARSITY_STATUS)) ;
    bool quick = false ;
//  printf ("here in %s %d : %d\n", __FILE__, __LINE__, base_offset) ;

    if (ncols == 1 && sparsity != GxB_HYPERSPARSE && fmt == GxB_BY_COL)
    { 
        // return S as a shallow GrB_Vector
        quick = true ;
        n = nrows ;
    }

    if (nrows == 1 && sparsity != GxB_HYPERSPARSE && fmt == GxB_BY_ROW)
    { 
        // quick in-place transpose, by converting it to by-column
        quick = true ;
        n = ncols ;
        S->is_csc = true ;
    }

    if (quick)
    {
        // return S as a shallow GrB_Vector, but subtract the base if needed
        ASSERT (GB_VECTOR_OK (S)) ;
        ASSERT_VECTOR_OK ((GrB_Vector) S, "S as vector", GB0) ;
        if (base_offset == 0)
        {
            V = (GrB_Vector) S ;
        }
        else
        { 
            // V = S - 1
            OK (GrB_Vector_new (&V, type, n)) ;
            ASSERT_VECTOR_OK (V, "V result, before apply", GB0) ;
            ASSERT_VECTOR_OK (S, "S before apply", GB0) ;
            OK (GrB_apply (V, NULL, NULL, GrB_MINUS_UINT64, (GrB_Vector) S, 1,
                NULL)) ;
            ASSERT_VECTOR_OK (V, "V result, after apply", GB0) ;
            GrB_Matrix_free (&S) ;
        }
//      printf ("here in %s %d\n", __FILE__, __LINE__) ;
        ASSERT_VECTOR_OK (V, "V result, quick", GB0) ;
//      printf ("here in %s %d\n", __FILE__, __LINE__) ;
        return (V) ;
    }
//  printf ("here in %s %d\n", __FILE__, __LINE__) ;

    //--------------------------------------------------------------------------
    // reshape S into (nrows*ncols)-by-1 and return it as a GrB_Vector
    //--------------------------------------------------------------------------

    bool ok = GB_uint64_multiply (&n, nrows, ncols) ;
    if (!ok)
    { 
        ERROR ("input matrix dimensions are too large") ;
    }
//  printf ("nrows %ld ncols %ld n %ld\n", nrows, ncols, n) ;

    OK (GxB_Matrix_reshapeDup (&C, S, true, n, 1, NULL)) ;
    GrB_Matrix_free (&S) ;
//  printf ("here in %s %d\n", __FILE__, __LINE__) ;

    // ensure C is not hypersparse, and is stored by column
    OK (GrB_set (C, GxB_SPARSE + GxB_BITMAP + GxB_FULL, GxB_SPARSITY_CONTROL)) ;
    OK (GrB_set (C, GxB_BY_COL, GxB_FORMAT)) ;

    // C is now a valid vector
    V = (GrB_Vector) C ;
    ASSERT (GB_VECTOR_OK (V)) ;
    ASSERT_VECTOR_OK (V, "C as vector", GB0) ;

    if (base_offset != 0)
    { 
        OK (GrB_apply (V, NULL, NULL, GrB_MINUS_UINT64, V, 1, NULL)) ;
    }

    // V is now a valid GrB_Vector (no longer shallow)
    ASSERT_VECTOR_OK (V, "V result, slow", GB0) ;
    return (V) ;
}


