//------------------------------------------------------------------------------
// GB_bitmap_assign_noM_accum_whole:  assign to C bitmap, mask M is not present
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------
// C<> += A            assign
// C<> += A            subassign

// C<repl> += A        assign
// C<repl> += A        subassign

// C<!> += A           assign: no work to do
// C<!> += A           subassign: no work to do

// C<!,repl> += A      assign: just clear C of all entries, not done here
// C<!,repl> += A      subassign: just clear C of all entries, not done here
//------------------------------------------------------------------------------

// C:           bitmap
// M:           none
// Mask_comp:   true or false
// Mask_struct: true or false
// C_replace:   true or false
// accum:       present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign or subassign (same action)

// If Mask_comp is true, then an empty mask is complemented.  This case has
// already been handled by GB_assign_prep, which calls GB_clear, and thus
// Mask_comp is always false in this method.

// JIT: needed.

#include "assign/GB_bitmap_assign_methods.h"
#define GB_GENERIC
#include "assign/include/GB_assign_shared_definitions.h"

#undef  GB_FREE_ALL
#define GB_FREE_ALL ;

GrB_Info GB_bitmap_assign_noM_accum_whole
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    const bool C_replace,       // descriptor for C
    #define I NULL              /* I index list */
    #define ni 0
    #define nI 0
    #define Ikind GB_ALL
    #define Icolon NULL
    #define J NULL              /* J index list */
    #define ni 0
    #define nI 0
    #define Jkind GB_ALL
    #define Jcolon NULL
    #define M NULL              /* mask matrix, not present here */
    const bool Mask_comp,       // true for !M, false for M
    const bool Mask_struct,     // true if M is structural, false if valued
    const GrB_BinaryOp accum,   // present
    const GrB_Matrix A,         // input matrix, not transposed
    const void *scalar,         // input scalar
    const GrB_Type scalar_type, // type of input scalar
    #define assign_kind         GB_ASSIGN
    GB_Werk Werk
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GBURBLE_BITMAP_ASSIGN ("bit5:whole", NULL, Mask_comp, accum,
        GB_ALL, GB_ALL, GB_ASSIGN) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, no M, accum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign, no M, accum", GB0) ;
    ASSERT_BINARYOP_OK (accum, "accum for bitmap assign, no M, accum", GB0) ; 

    int nthreads_max = GB_Context_nthreads_max ( ) ;
    double chunk = GB_Context_chunk ( ) ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    GB_GET_C_BITMAP ;           // C must be bitmap
    GB_GET_A_AND_SCALAR_FOR_BITMAP
    GB_GET_ACCUM_FOR_BITMAP

    //--------------------------------------------------------------------------
    // do the assignment
    //--------------------------------------------------------------------------

    if (!Mask_comp)
    {

        //----------------------------------------------------------------------
        // C += A or += scalar
        //----------------------------------------------------------------------

        if (A == NULL)
        { 

            //------------------------------------------------------------------
            // scalar assignment: C += scalar
            //------------------------------------------------------------------

            #undef  GB_CIJ_WORK
            #define GB_CIJ_WORK(pC)                         \
            {                                               \
                int8_t cb = Cb [pC] ;                       \
                if (cb == 0)                                \
                {                                           \
                    /* Cx [pC] = scalar */                  \
                    GB_COPY_cwork_to_C (Cx, pC, cwork, C_iso) ; \
                }                                           \
                else                                        \
                {                                           \
                    /* Cx [pC] += scalar */                 \
                    GB_ACCUMULATE_scalar (Cx, pC, ywork, C_iso) ;  \
                }                                           \
            }
            if (!C_iso)
            {
                #include "assign/factory/GB_bitmap_assign_C_whole_template.c"
            }

            // free the bitmap or set it to all ones
            GB_bitmap_assign_to_full (C, nthreads_max) ;

        }
        else
        {

            //------------------------------------------------------------------
            // matrix assignment: C += A
            //------------------------------------------------------------------

            if (GB_IS_FULL (A))
            { 

                //--------------------------------------------------------------
                // C += A where C is bitmap and A is full
                //--------------------------------------------------------------

                #undef  GB_CIJ_WORK
                #define GB_CIJ_WORK(pC)                                     \
                {                                                           \
                    int8_t cb = Cb [pC] ;                                   \
                    if (cb == 0)                                            \
                    {                                                       \
                        /* Cx [pC] = Ax [pC] */                             \
                        GB_COPY_aij_to_C (Cx,pC,Ax,pC,A_iso,cwork,C_iso) ;  \
                    }                                                       \
                    else                                                    \
                    {                                                       \
                        /* Cx [pC] += Ax [pC] */                            \
                        GB_ACCUMULATE_aij (Cx,pC,Ax,pC,A_iso,ywork,C_iso) ; \
                    }                                                       \
                }
                if (!C_iso)
                {
                    #include "assign/factory/GB_bitmap_assign_C_whole_template.c"
                }

                // free the bitmap or set it to all ones
                GB_bitmap_assign_to_full (C, nthreads_max) ;

            }
            else if (GB_IS_BITMAP (A))
            { 

                //--------------------------------------------------------------
                // C += A where C and A are bitmap
                //--------------------------------------------------------------

                #undef  GB_CIJ_WORK
                #define GB_CIJ_WORK(pC)                                        \
                {                                                              \
                    if (Ab [pC])                                               \
                    {                                                          \
                        int8_t cb = Cb [pC] ;                                  \
                        if (cb == 0)                                           \
                        {                                                      \
                            /* Cx [pC] = Ax [pC] */                            \
                            GB_COPY_aij_to_C (Cx,pC,Ax,pC,A_iso,cwork,C_iso) ; \
                            Cb [pC] = 1 ;                                      \
                            task_cnvals++ ;                                    \
                        }                                                      \
                        else                                                   \
                        {                                                      \
                            /* Cx [pC] += Ax [pC] */                           \
                            GB_ACCUMULATE_aij (Cx,pC,Ax,pC,A_iso,ywork,C_iso) ;\
                        }                                                      \
                    }                                                          \
                }
                #include "assign/factory/GB_bitmap_assign_C_whole_template.c"
                C->nvals = cnvals ;

            }
            else
            { 

                //--------------------------------------------------------------
                // C += A where C is bitmap and A is sparse or hyper
                //--------------------------------------------------------------

                #undef  GB_AIJ_WORK
                #define GB_AIJ_WORK(pC,pA)                                  \
                {                                                           \
                    int8_t cb = Cb [pC] ;                                   \
                    if (cb == 0)                                            \
                    {                                                       \
                        /* Cx [pC] = Ax [pA] */                             \
                        GB_COPY_aij_to_C (Cx,pC,Ax,pA,A_iso,cwork,C_iso) ;  \
                        Cb [pC] = 1 ;                                       \
                        task_cnvals++ ;                                     \
                    }                                                       \
                    else                                                    \
                    {                                                       \
                        /* Cx [pC] += Ax [pA] */                            \
                        GB_ACCUMULATE_aij (Cx,pC,Ax,pA,A_iso,ywork,C_iso) ; \
                    }                                                       \
                }
                #include "assign/factory/GB_bitmap_assign_A_whole_template.c"
                C->nvals = cnvals ;
            }
        }
    }

#if 0
    else if (C_replace)
    {
        // The mask is not present yet complemented: C_replace phase only.  all
        // entries are deleted.  This is done by GB_clear in GB_assign_prep
        // and is thus not needed here.
        GB_memset (Cb, 0, cnzmax, nthreads_max) ;
        cnvals = 0 ;
    }
#endif

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    ASSERT_MATRIX_OK (C, "C for bitmap assign, no M, accum, whole", GB0) ;
    return (GrB_SUCCESS) ;
}

