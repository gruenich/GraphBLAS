//------------------------------------------------------------------------------
// GB_matvec_set: set a field in a matrix or vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// FIXME: 32/64 matrix get/set

#include "get_set/GB_get_set.h"
#include "transpose/GB_transpose.h"
#define GB_FREE_ALL ;

GrB_Info GB_matvec_set
(
    GrB_Matrix A,
    bool is_vector,         // true if A is a GrB_Vector
    int ivalue,
    double dvalue,
    int field,
    GB_Werk Werk
)
{

    GrB_Info info ;
    GB_BURBLE_START ("GrB_set") ;
    ASSERT_MATRIX_OK (A, "A before set", GB0) ;

    int format = ivalue ;

    switch (field)
    {

        case GxB_HYPER_SWITCH : 

            if (is_vector)
            { 
                return (GrB_INVALID_VALUE) ;
            }
            A->hyper_switch = (float) dvalue ;
            break ;

        case GxB_HYPER_HASH : 

            A->no_hyper_hash = !((bool) ivalue) ;
            break ;

        case GxB_BITMAP_SWITCH : 

            A->bitmap_switch = (float) dvalue ;
            break ;

        case GxB_SPARSITY_CONTROL : 

            A->sparsity_control = GB_sparsity_control (ivalue, (int64_t) (-1)) ;
            break ;

        case GrB_STORAGE_ORIENTATION_HINT : 

            format = (ivalue == GrB_COLMAJOR) ? GxB_BY_COL : GxB_BY_ROW ;
            // fall through to the GxB_FORMAT case

        case GxB_FORMAT : 

            if (is_vector)
            { 
                // the hint is ignored
                return (GrB_SUCCESS) ;
            }
            if (! (format == GxB_BY_ROW || format == GxB_BY_COL))
            { 
                return (GrB_INVALID_VALUE) ;
            }
            bool new_csc = (format != GxB_BY_ROW) ;
            // conform the matrix to the new by-row/by-col format
            if (A->is_csc != new_csc)
            { 
                // A = A', done in-place, and change to the new format.
                GB_BURBLE_N (GB_nnz (A), "(transpose) ") ;
                GB_OK (GB_transpose_in_place (A, new_csc, Werk)) ;
                ASSERT (A->is_csc == new_csc) ;
                ASSERT (GB_JUMBLED_OK (A)) ;
            }
            break ;

//      case GxB_COLINDEX_INTEGER_HINT : // FIXME
//      case GxB_ROWINDEX_INTEGER_HINT : // FIXME
//      case GxB_COLINDEX_INTEGER_BITS : // FIXME
//      case GxB_ROWINDEX_INTEGER_BITS : // FIXME

        case GxB_OFFSET_INTEGER_HINT : 

            if (!(ivalue == 0 || ivalue == 32 || ivalue == 64))
            { 
                return (GrB_INVALID_VALUE) ;
            }
            if (ivalue == 32 && !A->p_is_32)
            { 
                // A->p is currently 64-bit; convert to 32-bit if possible
                GB_OK (GB_convert_int (A, true, A->j_is_32, A->i_is_32, true)) ;
            }
            else if (ivalue == 64 && A->i_is_32)
            { 
                // A->p is currently 32-bit; convert to 64-bit
                GB_OK (GB_convert_int (A, false, A->j_is_32, A->i_is_32, true)) ;
            }
            A->p_control = ivalue ;
            break ;

        case GxB_INDEX_INTEGER_HINT : 

            if (!(ivalue == 0 || ivalue == 32 || ivalue == 64))
            { 
                return (GrB_INVALID_VALUE) ;
            }
            if (ivalue == 32 && !A->i_is_32)
            { 
                // A->i is currently 64-bit; convert to 32-bit if possible
                GB_OK (GB_convert_int (A, A->p_is_32, A->j_is_32, true, true)) ;
            }
            else if (ivalue == 64 && A->i_is_32)
            { 
                // A->i is currently 32-bit; convert to 64-bit
                GB_OK (GB_convert_int (A, A->p_is_32, A->j_is_32, false, true)) ;
            }
            A->i_control = ivalue ;
            break ;

        default : 
            return (GrB_INVALID_VALUE) ;
    }

    //--------------------------------------------------------------------------
    // conform the matrix to its new desired sparsity structure
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (A, "A set before conform", GB0) ;
    GB_OK (GB_conform (A, Werk)) ;
    GB_BURBLE_END ;
    ASSERT_MATRIX_OK (A, "A set after conform", GB0) ;
    return (GrB_SUCCESS) ;
}

