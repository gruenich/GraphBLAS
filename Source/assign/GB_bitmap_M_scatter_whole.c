//------------------------------------------------------------------------------
// GB_bitmap_M_scatter_whole: scatter M into/from the C bitmap
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// This method only handles the full assign case, where there are not I and J
// index lists.  The C and M matrices must have the same size.

// JIT: not needed, but variants possible for each kind of mask matrix.
// This method is called from inside JIT kernels, so using a JIT for this
// method would be unusual.

// C is bitmap.  M is sparse or hypersparse, and may be jumbled.

#include "assign/GB_bitmap_assign_methods.h"
#define GB_GENERIC
#include "assign/include/GB_assign_shared_definitions.h"

GB_CALLBACK_BITMAP_M_SCATTER_WHOLE_PROTO (GB_bitmap_M_scatter_whole)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (M, "M for bitmap scatter, whole", GB0) ;
    ASSERT (GB_IS_BITMAP (C)) ;
    ASSERT (GB_IS_SPARSE (M) || GB_IS_HYPERSPARSE (M)) ;
    ASSERT (GB_JUMBLED_OK (M)) ;
    ASSERT (M_ntasks > 0) ;
    ASSERT (M_nthreads > 0) ;
    ASSERT (M_ek_slicing != NULL) ;

    //--------------------------------------------------------------------------
    // get C and M
    //--------------------------------------------------------------------------

    GB_GET_MASK
    int8_t *Cb = C->b ;
    const int64_t Cvlen = C->vlen ;
    int64_t cnvals = 0 ;    // not needed

    //--------------------------------------------------------------------------
    // scatter M into the C bitmap
    //--------------------------------------------------------------------------

    switch (operation)
    {

        case GB_BITMAP_M_SCATTER_PLUS_2 :       // Cb (i,j) += 2

            #undef  GB_MASK_WORK
            #define GB_MASK_WORK(pC) Cb [pC] += 2
            #include "template/GB_bitmap_assign_M_all_template.c"
            break ;

        case GB_BITMAP_M_SCATTER_MINUS_2 :      // Cb (i,j) -= 2

            #undef  GB_MASK_WORK
            #define GB_MASK_WORK(pC) Cb [pC] -= 2
            #include "template/GB_bitmap_assign_M_all_template.c"
            break ;

        case GB_BITMAP_M_SCATTER_SET_2 :        // Cb (i,j) = 2

            #undef  GB_MASK_WORK
            #define GB_MASK_WORK(pC) Cb [pC] = 2
            #include "template/GB_bitmap_assign_M_all_template.c"
            break ;

        default: ;
    }
}

