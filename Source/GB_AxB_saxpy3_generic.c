//------------------------------------------------------------------------------
// GB_AxB_saxpy3_generic: compute C=A*B, C<M>=A*B, or C<!M>=A*B in parallel
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// GB_AxB_saxpy3_generic computes C=A*B, C<M>=A*B, or C<!M>=A*B in parallel,
// with arbitrary types and operators.

//------------------------------------------------------------------------------

#include "GB_mxm.h"
#include "GB_binop.h"
#include "GB_AxB_saxpy3.h"
#include "GB_bracket.h"
#include "GB_sort.h"
#include "GB_atomics.h"

GrB_Info GB_AxB_saxpy3_generic
(
    GrB_Matrix C,
    const GrB_Matrix M, bool Mask_comp, const bool Mask_struct,
    const bool M_dense_in_place,
    const GrB_Matrix A, bool A_is_pattern,
    const GrB_Matrix B, bool B_is_pattern,
    const GrB_Semiring semiring,    // semiring that defines C=A*B
    const bool flipxy,              // if true, do z=fmult(b,a) vs fmult(a,b)
    GB_saxpy3task_struct *GB_RESTRICT TaskList,
    const int ntasks,
    const int nfine,
    const int nthreads,
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // get operators, functions, workspace, contents of A, B, and C
    //--------------------------------------------------------------------------

    GrB_BinaryOp mult = semiring->multiply ;
    GrB_Monoid add = semiring->add ;
    ASSERT (mult->ztype == add->op->ztype) ;

    GxB_binary_function fmult = mult->function ;    // NULL if positional
    GxB_binary_function fadd  = add->op->function ;
    GB_Opcode opcode = mult->opcode ;
    bool op_is_positional = GB_OPCODE_IS_POSITIONAL (opcode) ;

    size_t csize = C->type->size ;
    size_t asize = A_is_pattern ? 0 : A->type->size ;
    size_t bsize = B_is_pattern ? 0 : B->type->size ;

    size_t xsize = mult->xtype->size ;
    size_t ysize = mult->ytype->size ;

    // scalar workspace: because of typecasting, the x/y types need not
    // be the same as the size of the A and B types.
    // flipxy false: aik = (xtype) A(i,k) and bkj = (ytype) B(k,j)
    // flipxy true:  aik = (ytype) A(i,k) and bkj = (xtype) B(k,j)
    size_t aik_size = flipxy ? ysize : xsize ;
    size_t bkj_size = flipxy ? xsize : ysize ;

    GB_void *GB_RESTRICT terminal = (GB_void *) add->terminal ;
    GB_void *GB_RESTRICT identity = (GB_void *) add->identity ;

    GB_cast_function cast_A, cast_B ;
    if (flipxy)
    { 
        // A is typecasted to y, and B is typecasted to x
        cast_A = A_is_pattern ? NULL : 
                 GB_cast_factory (mult->ytype->code, A->type->code) ;
        cast_B = B_is_pattern ? NULL : 
                 GB_cast_factory (mult->xtype->code, B->type->code) ;
        if (op_is_positional)
        {
            // flip a positional multiplicative operator
            opcode = GB_binop_flip (opcode) ;
        }
    }
    else
    { 
        // A is typecasted to x, and B is typecasted to y
        cast_A = A_is_pattern ? NULL :
                 GB_cast_factory (mult->xtype->code, A->type->code) ;
        cast_B = B_is_pattern ? NULL :
                 GB_cast_factory (mult->ytype->code, B->type->code) ;
    }

    //--------------------------------------------------------------------------
    // C = A*B via saxpy3 method, function pointers, and typecasting
    //--------------------------------------------------------------------------

    // memcpy (&(Cx [pC]), &(Hx [i]), len*csize)
    #define GB_CIJ_MEMCPY(pC,i,len) \
        memcpy (GB_CX (pC), GB_HX (i), (len)*csize)

    // atomic update not available for function pointers
    #define GB_HAS_ATOMIC 0

    // 1 if monoid update can be skipped entirely (the ANY monoid)
    #define GB_IS_ANY_MONOID 0

    // user-defined monoid update cannot be done with an OpenMP atomic
    #define GB_HAS_OMP_ATOMIC 0

    // not an ANY_PAIR semiring
    #define GB_IS_ANY_PAIR_SEMIRING 0

    // not a PAIR multiply operator 
    #define GB_IS_PAIR_MULTIPLIER 0

    #define GB_ATYPE GB_void
    #define GB_BTYPE GB_void

    // no vectorization
    #define GB_PRAGMA_SIMD_VECTORIZE ;

    // definitions for GB_AxB_saxpy3_template.c
    #include "GB_AxB_saxpy3_template.h"

    if (op_is_positional)
    {

        //----------------------------------------------------------------------
        // generic semirings with positional mulitiply operators
        //----------------------------------------------------------------------

        GB_BURBLE_MATRIX (C, "(generic positional C=A*B) ") ;

        // C always has type int64_t.  The monoid must be used via its function
        // pointer.  The positional multiply operator must be hard-coded since
        // it has no function pointer.

        // aik = A(i,k), located in Ax [pA], value not used
        #define GB_GETA(aik,Ax,pA) ;

        // bkj = B(k,j), located in Bx [pB], value not used
        #define GB_GETB(bkj,Bx,pB) ;

        // define t for each task
        #define GB_CIJ_DECLARE(t) GB_CTYPE t

        // address of Cx [p]
        #define GB_CX(p) (&Cx [p])

        // Cx [p] = t
        #define GB_CIJ_WRITE(p,t) Cx [p] = t

        // address of Hx [i]
        #define GB_HX(i) (&Hx [i])

        // Hx [i] = t
        #define GB_HX_WRITE(i, t) Hx [i] = t

        // Cx [p] = Hx [i]
        #define GB_CIJ_GATHER(p,i) Cx [p] = Hx [i]

        // Cx [p] += t
        #define GB_CIJ_UPDATE(p,t) fadd (GB_CX (p), GB_CX (p), &t)

        // Hx [i] += t
        #define GB_HX_UPDATE(i, t) fadd (GB_HX (i), GB_HX (i), &t)

        #define GB_IDENTITY add_identity

        int64_t offset = GB_positional_offset (opcode) ;

        if (mult->ztype == GrB_INT64)
        {
            // monoid identity value
            int64_t add_identity ;
            memcpy (&add_identity, identity, sizeof (int64_t)) ;
            #undef  GB_CTYPE
            #define GB_CTYPE int64_t
            ASSERT (C->type == GrB_INT64) ;
            ASSERT (csize == sizeof (int64_t)) ;
            switch (opcode)
            {
                case GB_FIRSTI_opcode   :   // z = first_i(A(i,k),y) == i
                case GB_FIRSTI1_opcode  :   // z = first_i1(A(i,k),y) == i+1
                    #undef  GB_MULT
                    #define GB_MULT(t, aik, bkj, i, k, j) t = i + offset
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                case GB_FIRSTJ_opcode   :   // z = first_j(A(i,k),y) == k
                case GB_FIRSTJ1_opcode  :   // z = first_j1(A(i,k),y) == k+1
                case GB_SECONDI_opcode  :   // z = second_i(x,B(k,j)) == k
                case GB_SECONDI1_opcode :   // z = second_i1(x,B(k,j)) == k+1
                    #undef  GB_MULT
                    #define GB_MULT(t, aik, bkj, i, k, j) t = k + offset
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                case GB_SECONDJ_opcode  :   // z = second_j(x,B(k,j)) == j
                case GB_SECONDJ1_opcode :   // z = second_j1(x,B(k,j)) == j+1
                    #undef  GB_MULT
                    #define GB_MULT(t, aik, bkj, i, k, j) t = j + offset
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                default: ;
            }
        }
        else
        {
            // monoid identity value
            int32_t add_identity ;
            memcpy (&add_identity, identity, sizeof (int32_t)) ;
            #undef  GB_CTYPE
            #define GB_CTYPE int32_t
            ASSERT (C->type == GrB_INT32) ;
            ASSERT (csize == sizeof (int32_t)) ;
            switch (opcode)
            {
                case GB_FIRSTI_opcode   :   // z = first_i(A(i,k),y) == i
                case GB_FIRSTI1_opcode  :   // z = first_i1(A(i,k),y) == i+1
                    #undef  GB_MULT
                    #define GB_MULT(t,aik,bkj,i,k,j) t = (int32_t) (i + offset)
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                case GB_FIRSTJ_opcode   :   // z = first_j(A(i,k),y) == k
                case GB_FIRSTJ1_opcode  :   // z = first_j1(A(i,k),y) == k+1
                case GB_SECONDI_opcode  :   // z = second_i(x,B(k,j)) == k
                case GB_SECONDI1_opcode :   // z = second_i1(x,B(k,j)) == k+1
                    #undef  GB_MULT
                    #define GB_MULT(t,aik,bkj,i,k,j) t = (int32_t) (k + offset)
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                case GB_SECONDJ_opcode  :   // z = second_j(x,B(k,j)) == j
                case GB_SECONDJ1_opcode :   // z = second_j1(x,B(k,j)) == j+1
                    #undef  GB_MULT
                    #define GB_MULT(t,aik,bkj,i,k,j) t = (int32_t) (j + offset)
                    #include "GB_AxB_saxpy3_template.c"
                    break ;
                default: ;
            }
        }

    }
    else
    {

        //----------------------------------------------------------------------
        // generic semirings with standard mulitiply operators
        //----------------------------------------------------------------------

        GB_BURBLE_MATRIX (C, "(generic C=A*B) ") ;

        // monoid identity value
        #undef  GB_IDENTITY
        #define GB_IDENTITY identity

        // aik = A(i,k), located in Ax [pA]
        #undef  GB_GETA
        #define GB_GETA(aik,Ax,pA)                                          \
            GB_void aik [GB_VLA(aik_size)] ;                                \
            if (!A_is_pattern) cast_A (aik, Ax +((pA)*asize), asize)

        // bkj = B(k,j), located in Bx [pB]
        #undef  GB_GETB
        #define GB_GETB(bkj,Bx,pB)                                          \
            GB_void bkj [GB_VLA(bkj_size)] ;                                \
            if (!B_is_pattern) cast_B (bkj, Bx +((pB)*bsize), bsize)

        // define t for each task
        #undef  GB_CIJ_DECLARE
        #define GB_CIJ_DECLARE(t) GB_void t [GB_VLA(csize)]

        // address of Cx [p]
        #undef  GB_CX
        #define GB_CX(p) (Cx +((p)*csize))

        // Cx [p] = t
        #undef  GB_CIJ_WRITE
        #define GB_CIJ_WRITE(p,t) memcpy (GB_CX (p), t, csize)

        // address of Hx [i]
        #undef  GB_HX
        #define GB_HX(i) (Hx +((i)*csize))

        // Hx [i] = t
        #undef  GB_HX_WRITE
        #define GB_HX_WRITE(i, t) memcpy (GB_HX (i), t, csize)

        // Cx [p] = Hx [i]
        #undef  GB_CIJ_GATHER
        #define GB_CIJ_GATHER(p,i) memcpy (GB_CX (p), GB_HX(i), csize)

        // Cx [p] += t
        #undef  GB_CIJ_UPDATE
        #define GB_CIJ_UPDATE(p,t) fadd (GB_CX (p), GB_CX (p), t)

        // Hx [i] += t
        #undef  GB_HX_UPDATE
        #define GB_HX_UPDATE(i, t) fadd (GB_HX (i), GB_HX (i), t)

        #undef  GB_CTYPE
        #define GB_CTYPE GB_void

        if (flipxy)
        { 
            // t = B(k,j) * A(i,k)
            #undef  GB_MULT
            #define GB_MULT(t, aik, bkj, i, k, j) fmult (t, bkj, aik)
            #include "GB_AxB_saxpy3_template.c"
        }
        else
        { 
            // t = A(i,k) * B(k,j)
            #undef  GB_MULT
            #define GB_MULT(t, aik, bkj, i, k, j) fmult (t, aik, bkj)
            #include "GB_AxB_saxpy3_template.c"
        }
    }

    return (GrB_SUCCESS) ;
}

