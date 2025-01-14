//------------------------------------------------------------------------------
// GB_ijvector: extract a list of indices from a GrB_Vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// The input vector List describes a list of integers to be used by GrB_assign,
// GxB_subassign, or GrB_extract.
#define GB_DEBUG

#include "GB_ij.h"
#include "matrix/include/GB_static_header.h"
#define GB_FREE_ALL                 \
{                                   \
    GB_FREE (&I2, I2_size) ;        \
    if (I != NULL && I_size > 0)    \
    {                               \
        GB_FREE (&I, I_size) ;      \
    }                               \
    GB_Matrix_free (&T) ;           \
}

GrB_Info GB_ijvector
(
    // input:
    GrB_Vector List,        // defines the list of integers, either from
                            // List->x or List-i.  If List is NULL, it defines
                            // I = GrB_ALL.
    bool List_values,       // if true, I is determined by List->x;
                            // otherwise, I is determined by List->i.
    bool need_copy,         // if true, I must be allocated
    // output:
    void **I_handle,        // the list I; may be GrB_ALL
    bool *I_is_32_handle,   // if true, I is 32-bit; else 64-bit
    int64_t *ni_handle,     // the length of I, or special (GxB_RANGE)
    size_t *I_size_handle,  // if > 0, I has been allocated by this
                            // method.  Otherwise, it is a shallow pointer into
                            // List->x or List->i.
    GB_Werk Werk                            
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT (I_handle != NULL) ;
    ASSERT (I_is_32_handle != NULL) ;
    ASSERT (ni_handle != NULL) ;
    ASSERT (I_size_handle != NULL) ;
    ASSERT_VECTOR_OK_OR_NULL (List, "List of integers", GB0) ;

    (*I_handle) = NULL ;
    (*I_is_32_handle) = false ;
    (*ni_handle) = 0 ;
    (*I_size_handle) = 0 ;

    struct GB_Matrix_opaque T_header ;
    GrB_Matrix T = NULL ;
    size_t I_size = 0, I2_size = 0 ;
    void *I = NULL, *I2 = NULL ;

    //--------------------------------------------------------------------------
    // quick return if List is NULL
    //--------------------------------------------------------------------------

    if (List == NULL)
    { 
        // List of NULL denotes GrB_ALL, or ":"
        (*I_handle) = (uint64_t *) GrB_ALL ;
        return (GrB_SUCCESS) ;
    }

    //--------------------------------------------------------------------------
    // finish any pending work in the List
    //--------------------------------------------------------------------------

    GB_MATRIX_WAIT (List) ;

    //--------------------------------------------------------------------------
    // quick return if List is empty
    //--------------------------------------------------------------------------

    int64_t ni = List->nvals ;
    if (ni == 0)
    { 
        // List is empty
        (*I_handle) = GB_CALLOC_MEMORY (1, sizeof (uint64_t), I_size_handle) ;
        if ((*I_handle) == NULL)
        { 
            return (GrB_OUT_OF_MEMORY) ;
        }
        return (GrB_SUCCESS) ;
    }

    //--------------------------------------------------------------------------
    // extract the list of integers from the List vector
    //--------------------------------------------------------------------------

    int List_sparsity = GB_sparsity (List) ;
    GrB_Type I_type = NULL ;
    bool iso = false ;

    if (List_sparsity == GxB_SPARSE)
    {

        //----------------------------------------------------------------------
        // List is sparse
        //----------------------------------------------------------------------

        if (List_values)
        { 
            I = List->x ;
            I_type = List->type ;
            iso = List->iso ;
        }
        else
        { 
            I = List->i ;
            I_type = (List->i_is_32) ? GrB_UINT32 : GrB_UINT64 ;
        }

    }
    else if (List_sparsity == GxB_BITMAP)
    {

        //----------------------------------------------------------------------
        // List is bitmap
        //----------------------------------------------------------------------

        uint64_t Cp [2] ;
        if (List_values)
        { 
            if (List->iso)
            { 
                // get the iso value
                I = List->x ;
                iso = true ;
            }
            else
            { 
                // extract the values from the bitmap vector
                I = GB_MALLOC_MEMORY (ni, List->type->size, &I_size) ;
                if (I == NULL)
                { 
                    // out of memory
                    GB_FREE_ALL ;
                    return (GrB_OUT_OF_MEMORY) ;
                }
                GB_OK (GB_convert_b2s (Cp, NULL, NULL, /* Cx: */ I, NULL,
                    false, false, false, List->type, (GrB_Matrix) List, Werk)) ;
            }
            I_type = List->type ;
        }
        else
        { 
            // extract the indices from the bitmap vector
            I_type = (ni <= UINT32_MAX) ? GrB_UINT32 : GrB_UINT64 ;
            I = GB_MALLOC_MEMORY (ni, I_type->size, &I_size) ;
            if (I == NULL)
            { 
                // out of memory
                GB_FREE_ALL ;
                return (GrB_OUT_OF_MEMORY) ;
            }
            GB_OK (GB_convert_b2s (Cp, /* Ci: */ I, NULL, NULL, NULL,
                false, I_type == GrB_UINT32, false, List->type,
                (GrB_Matrix) List, Werk)) ;
        }

    }
    else // List_sparsity == GxB_FULL
    { 

        //----------------------------------------------------------------------
        // List is full
        //----------------------------------------------------------------------

        if (List_values)
        { 
            I = List->x ;
            I_type = List->type ;
            iso = List->iso ;
        }
        else
        { 
            // I = 0:length(List)-1
            I = GB_CALLOC_MEMORY (3, sizeof (uint64_t), &I_size) ;
            if (I == NULL)
            { 
                // out of memory
                GB_FREE_ALL ;
                return (GrB_OUT_OF_MEMORY) ;
            }
            uint64_t *I64 = (uint64_t *) I ;
            I64 [1] = List->vlen - 1 ;
            I_type = GrB_UINT64 ;
            ni = GxB_RANGE ;
        }
    }

    //--------------------------------------------------------------------------
    // expand I if it is iso-valued
    //--------------------------------------------------------------------------

    if (iso)
    { 
        I2 = GB_MALLOC_MEMORY (ni, I_type->size, &I2_size) ;
        if (I2 == NULL)
        { 
            // out of memory
            GB_FREE_ALL ;
            return (GrB_OUT_OF_MEMORY) ;
        }
        GB_OK (GB_iso_expand (I2, ni, I, I_type)) ;
        // free the old I and replace it with I2
        if (I_size > 0)
        { 
            GB_FREE (&I, I_size) ;
        }
        I = I2 ;
        I_size = I2_size ;
        I2 = NULL ;
        I2_size = 0 ;
    }

    //--------------------------------------------------------------------------
    // copy/typecast the indices if needed
    //--------------------------------------------------------------------------

    bool need_typecast = false ;

    if (I_type == GrB_INT32 || I_type == GrB_UINT32)
    { 
        // implicit typecast of int32_t to uint32_t (I does not change)
        I_type = GrB_UINT32 ;
    }
    else if (I_type == GrB_INT64 || I_type == GrB_UINT64)
    { 
        // implicit typecast of int64_t to uint64_t (I does not change)
        I_type = GrB_UINT64 ;
    }
    else
    { 
        need_typecast = true ;
    }

    if (need_copy || need_typecast)
    { 
        // Create an n-by-1 matrix T containing the values of I
        int64_t n = (iso) ? 1 : ni ;
        GB_CLEAR_STATIC_HEADER (T, &T_header) ;
        GB_OK (GB_new (&T, // static header
            I_type, n, 1, GB_ph_null, true, GxB_FULL, 0, 0,
            false, false, false)) ;
        T->x = I ;
        T->x_shallow = true ;
        ASSERT_MATRIX_OK (T, "T for typecast to I", GB0) ;

        // I2 = (uint64_t) T->x
        I2 = GB_MALLOC_MEMORY (n, sizeof (uint64_t), &I2_size) ;
        if (I2 == NULL)
        { 
            // out of memory
            GB_FREE_ALL ;
            return (GrB_OUT_OF_MEMORY) ;
        }
        int nthreads_max = GB_Context_nthreads_max ( ) ;
        GB_OK (GB_cast_array (I2, GB_UINT64_code, T, nthreads_max)) ;
        GB_Matrix_free (&T) ;

        // free the old I and replace it with I2
        if (I_size > 0)
        { 
            GB_FREE (&I, I_size) ;
        }
        I = I2 ;
        I_size = I2_size ;
        I2 = NULL ;
        I2_size = 0 ;
        I_type = GrB_UINT64 ;
    }

    ASSERT (I_type == GrB_UINT32 || I_type == GrB_UINT64) ;
    bool I_is_32 = (I_type == GrB_UINT32) ;

    //--------------------------------------------------------------------------
    // free workspace return result
    //--------------------------------------------------------------------------

    (*I_handle) = I ;
    (*I_is_32_handle) = I_is_32 ;
    (*ni_handle) = ni ;
    (*I_size_handle) = I_size ;
    I = NULL ;
    GB_FREE_ALL ;
    return (GrB_SUCCESS) ;
}

