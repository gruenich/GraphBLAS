//------------------------------------------------------------------------------
// gb_export_to_mxstruct: export a GrB_Matrix to a MATLAB struct
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// The input GrB_Matrix A is exported to a GraphBLAS matrix struct G, and freed.

// The input GrB_Matrix A must be deep.  The output is a MATLAB struct
// holding the content of the GrB_Matrix.

// The GraphBLASv4 and v5 structs are identical, except that s has size 9
// in v4 and size 10 in v5.  The added s [9] entry is true if the matrix is
// uniform valued.  If the matrix is uniform-valued, the x array is only
// large enough to hold a single entry.

// GraphBLASv5 and GraphBLASv5_1 are identical, except that s [9] is present
// but always false for GraphBLASv5.

// GraphBLASv7_3 is identical to GraphBLASv5_1, except that it adds the Y
// hyper_hash with 3 components: Yp, Yi, and Yx.

// GraphBLASv10 is identical to GraphBLASv7_3, except that Ap, Ah, Ai, Yp, Yi,
// and Yx can be uint32_t.  The fields are the same as GraphBLASv7_3.

// mxGetData and mxSetData are used instead of the MATLAB-recommended
// mxGetDoubles, etc, because mxGetData and mxSetData work best for Octave, and
// they work fine for MATLAB since GraphBLAS requires R2018a with the
// interleaved complex data type.

#include "gb_interface.h"

// for hypersparse, sparse, or full matrices
static const char *MatrixFields [9] =
{
    // these fields are identical to GraphBLASv5_1, except for the name
    // of the first field
    "GraphBLASv10",     // 0: "logical", "int8", ... "double",
                        //    "single complex", or "double complex"
    "s",                // 1: all scalar info goes here
    "x",                // 2: array of uint8, size (sizeof(type)*nzmax), or
                        //    just sizeof(type) if the matrix is uniform-valued
    "p",                // 3: array of uint32_t or uint64_t, size plen+1
    "i",                // 4: array of uint32_t or uint64_t, size nzmax
    "h",                // 5: array of uint32_t or uint64_t, size plen if hyper
    // added for v7.2: for hypersparse matrices only:
    "Yp",               // 6: Y->p, uint32_t or uint64_t array, size Y->vdim+1
    "Yi",               // 7: Y->i, uint32_t or uint64_t array, size nvec (s[3])
    "Yx"                // 8: Y->x, uint32_t or uint64_t array, size nvec
} ;

// for bitmap matrices only
static const char *Bitmap_MatrixFields [4] =
{
    "GraphBLASv10",     // 0: "logical", "int8", ... "double",
                        //    "single complex", or "double complex"
    "s",                // 1: all scalar info goes here
    "x",                // 2: array of uint8, size (sizeof(type)*nzmax), or
                        //    just sizeof(type) if the matrix is uniform-valued
    "b"                 // 3: array of int8_t, size nzmax, for bitmap only
} ;

//------------------------------------------------------------------------------

mxArray *gb_export_to_mxstruct  // return exported MATLAB struct G
(
    GrB_Matrix *A_handle        // matrix to export; freed on output
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    CHECK_ERROR (A_handle == NULL, "matrix missing") ;

    GrB_Matrix T = NULL ;
    if (GB_is_shallow (*A_handle))
    {
        // A is shallow so make a deep copy
        OK (GrB_Matrix_dup (&T, *A_handle)) ;
        OK (GrB_Matrix_free (A_handle)) ;
        (*A_handle) = T ;
    }

    GrB_Matrix A = (*A_handle) ;
    GrB_Matrix Y = NULL ;

    //--------------------------------------------------------------------------
    // make sure the matrix is finished, including the creation of A->Y
    //--------------------------------------------------------------------------

    OK1 (A, GrB_Matrix_wait (A, GrB_MATERIALIZE)) ;

OK (GxB_Matrix_fprint (A, "A to export", 2, NULL)) ;    // FIXME

    //--------------------------------------------------------------------------
    // extract the content of the GrB_Matrix and free it
    //--------------------------------------------------------------------------

    int sparsity_status ;
    OK (GrB_Matrix_get_INT32 (A, &sparsity_status, GxB_SPARSITY_STATUS)) ;
    int sparsity_control = A->sparsity_control ;
// printf ("sparsity_ status export %d\n", sparsity_status) ;  // FIXME

    int64_t nzmax = GB_nnz_max (A) ;
    int64_t plen = A->plen ;
    int64_t nvec_nonempty = A->nvec_nonempty ;

    GrB_Type type = A->type ;
    size_t type_size = type->size ;
    int64_t vlen = A->vlen ;
    int64_t vdim = A->vdim ;

    void   *Ap = A->p ; A->p = NULL ;
    void   *Ah = A->h ; A->h = NULL ;
    int8_t *Ab = A->b ; A->b = NULL ;
    void   *Ai = A->i ; A->i = NULL ;
    int8_t *Ax = A->x ; A->x = NULL ;

    uint64_t Ap_size = A->p_size ;
    uint64_t Ah_size = A->h_size ;
    uint64_t Ab_size = A->b_size ;
    uint64_t Ai_size = A->i_size ;
    uint64_t Ax_size = A->x_size ;

    uint64_t nvals = A->nvals ;
    uint64_t nvec = A->nvec ;
    bool by_col = A->is_csc ;
    bool iso = A->iso ;

    bool Ap_is_32 = A->p_is_32 ;
    bool Aj_is_32 = A->j_is_32 ;
    bool Ai_is_32 = A->i_is_32 ;

    mxClassID Ap_class = Ap_is_32 ? mxUINT32_CLASS : mxUINT64_CLASS ;
    mxClassID Aj_class = Aj_is_32 ? mxUINT32_CLASS : mxUINT64_CLASS ;
    mxClassID Ai_class = Ai_is_32 ? mxUINT32_CLASS : mxUINT64_CLASS ;

    size_t psize = Ap_is_32 ? sizeof (uint32_t) : sizeof (uint64_t) ;
    size_t jsize = Aj_is_32 ? sizeof (uint32_t) : sizeof (uint64_t) ;
    size_t isize = Ai_is_32 ? sizeof (uint32_t) : sizeof (uint64_t) ;

    GrB_Type ytype = NULL ;
    void *Yp = NULL ; uint64_t Yp_size = 0 ;
    void *Yi = NULL ; uint64_t Yi_size = 0 ;
    void *Yx = NULL ; uint64_t Yx_size = 0 ;
    uint64_t yvdim = 0, yvlen = 0 ;
    if (A->Y != NULL)
    { 
        Yp = A->Y->p ; A->Y->p = NULL ;
        Yi = A->Y->i ; A->Y->i = NULL ;
        Yx = A->Y->x ; A->Y->x = NULL ;
        Yp_size = A->Y->p_size ;
        Yi_size = A->Y->i_size ;
        Yx_size = A->Y->x_size ;
        yvdim = A->Y->vdim ;
        yvlen = A->Y->vlen ;
    }

    GrB_Matrix_free (&A) ;

    //--------------------------------------------------------------------------
    // construct the output struct
    //--------------------------------------------------------------------------

    mxArray *G ;
    switch (sparsity_status)
    {
        case GxB_FULL :
            // A is full, with 3 fields: GraphBLAS*, s, x
            G = mxCreateStructMatrix (1, 1, 3, MatrixFields) ;
            break ;

        case GxB_SPARSE :
            // A is sparse, with 5 fields: GraphBLAS*, s, x, p, i
            G = mxCreateStructMatrix (1, 1, 5, MatrixFields) ;
            break ;

        case GxB_HYPERSPARSE :
            // A is hypersparse, with 6 or 9 fields: GraphBLAS*, s, x, p, i, h,
            // Yp, Yi, Yx
            G = mxCreateStructMatrix (1, 1, (Yp == NULL) ? 6 : 9,
                MatrixFields) ;
            break ;

        case GxB_BITMAP :
            // A is bitmap, with 4 fields: GraphBLAS*, s, x, b
            G = mxCreateStructMatrix (1, 1, 4, Bitmap_MatrixFields) ;
            break ;

        default : ERROR ("invalid GraphBLAS struct") ;
    }

    //--------------------------------------------------------------------------
    // export content into the output struct
    //--------------------------------------------------------------------------

    // export the GraphBLAS type as a string
    mxSetFieldByNumber (G, 0, 0, gb_type_to_mxstring (type)) ;

    // export the scalar content
    mxArray *opaque = mxCreateNumericMatrix (1, 10, mxINT64_CLASS, mxREAL) ;
    int64_t *s = (int64_t *) mxGetData (opaque) ;
    s [0] = plen ;
    s [1] = vlen ;
    s [2] = vdim ;
    s [3] = (sparsity_status == GxB_HYPERSPARSE) ? nvec : (s [2]) ;
    s [4] = nvec_nonempty ;
    s [5] = sparsity_control ;
    s [6] = (int64_t) by_col ;
    s [7] = nzmax ;
    s [8] = nvals ;
    s [9] = (int64_t) iso ;             // new in GraphBLASv5
    mxSetFieldByNumber (G, 0, 1, opaque) ;

    // These components do not need to be exported: Pending, nzombies,
    // queue_next, queue_head, enqueued, *_shallow, jumbled, logger,
    // hyper_switch, bitmap_switch.

    if (sparsity_status == GxB_SPARSE || sparsity_status == GxB_HYPERSPARSE)
    {
        // export the pointers
        mxArray *Ap_mx = mxCreateNumericMatrix (1, 0, Ap_class, mxREAL) ;
        mxSetN (Ap_mx, Ap_size / psize) ;
        void *p = (void *) mxGetData (Ap_mx) ; gb_mxfree (&p) ;
        mxSetData (Ap_mx, Ap) ;
        GBMDUMP ("gb_export, remove Ap from memtable %p\n", Ap) ;
        GB_Global_memtable_remove (Ap) ; Ap = NULL ;
        mxSetFieldByNumber (G, 0, 3, Ap_mx) ;

        // export the indices
        mxArray *Ai_mx = mxCreateNumericMatrix (1, 0, Ai_class, mxREAL) ;
        if (Ai_size > 0)
        { 
            mxSetN (Ai_mx, Ai_size / isize) ;
            p = (void *) mxGetData (Ai_mx) ; gb_mxfree (&p) ;
            mxSetData (Ai_mx, Ai) ;
            GBMDUMP ("gb_export, remove Ai from memtable %p\n", Ai) ;
            GB_Global_memtable_remove (Ai) ; Ai = NULL ;
        }
        mxSetFieldByNumber (G, 0, 4, Ai_mx) ;
    }

    // export the values as uint8
    mxArray *Ax_mx = mxCreateNumericMatrix (1, 0, mxUINT8_CLASS, mxREAL) ;
    if (Ax_size > 0)
    { 
        mxSetN (Ax_mx, Ax_size) ;
        void *p = mxGetData (Ax_mx) ; gb_mxfree (&p) ;
        mxSetData (Ax_mx, Ax) ;
        GBMDUMP ("gb_export, remove Ax from memtable %p\n", Ax) ;
        GB_Global_memtable_remove (Ax) ; Ax = NULL ;
    }
    mxSetFieldByNumber (G, 0, 2, Ax_mx) ;

    if (sparsity_status == GxB_HYPERSPARSE)
    {
        // export the hyperlist
        mxArray *Ah_mx = mxCreateNumericMatrix (1, 0, Aj_class, mxREAL) ;
        if (Ah_size > nvec * jsize)
        {
            // clear the space beyond the end of the data
            memset (Ah + nvec, 0, Ah_size - nvec * jsize) ;
        }
        if (Ah_size > 0)
        { 
            mxSetN (Ah_mx, Ah_size / jsize) ;
            void *p = (void *) mxGetData (Ah_mx) ; gb_mxfree (&p) ;
            mxSetData (Ah_mx, Ah) ;
            GBMDUMP ("gb_export, remove Ah from memtable %p\n", Ah) ;
            GB_Global_memtable_remove (Ah) ; Ah = NULL ;
        }
        mxSetFieldByNumber (G, 0, 5, Ah_mx) ;

        if (Yp != NULL)
        {

            // export Yp, of size yvdim+1
            mxArray *Yp_mx = mxCreateNumericMatrix (1, 0, Aj_class, mxREAL) ;
            mxSetN (Yp_mx, yvdim+1) ;
            void *p = (void *) mxGetData (Yp_mx) ; gb_mxfree (&p) ;
            mxSetData (Yp_mx, Yp) ;
            GBMDUMP ("gb_export, remove Yp from memtable %p\n", Yp) ;
            GB_Global_memtable_remove (Yp) ; Yp = NULL ;
            mxSetFieldByNumber (G, 0, 6, Yp_mx) ;

            // export Yi, of size nvec
            mxArray *Yi_mx = mxCreateNumericMatrix (1, 0, Aj_class, mxREAL) ;
            mxSetN (Yi_mx, nvec) ;
            p = (void *) mxGetData (Yi_mx) ; gb_mxfree (&p) ;
            mxSetData (Yi_mx, Yi) ;
            GBMDUMP ("gb_export, remove Yi from memtable %p\n", Yi) ;
            GB_Global_memtable_remove (Yi) ; Yi = NULL ;
            mxSetFieldByNumber (G, 0, 7, Yi_mx) ;

            // export Yx, of size nvec
            mxArray *Yx_mx = mxCreateNumericMatrix (1, 0, Aj_class, mxREAL) ;
            mxSetN (Yx_mx, nvec) ;
            p = (void *) mxGetData (Yx_mx) ; gb_mxfree (&p) ;
            mxSetData (Yx_mx, Yx) ;
            GBMDUMP ("gb_export, remove Yx from memtable %p\n", Yx) ;
            GB_Global_memtable_remove (Yx) ; Yx = NULL ;
            mxSetFieldByNumber (G, 0, 8, Yx_mx) ;
        }
    }

    if (sparsity_status == GxB_BITMAP)
    { 
        // export the bitmap
        mxArray *Ab_mx = mxCreateNumericMatrix (1, 0, mxINT8_CLASS, mxREAL) ;
        if (Ab_size > 0)
        { 
            mxSetN (Ab_mx, Ab_size) ;
            void *p = (void *) mxGetData (Ab_mx) ; gb_mxfree (&p) ;
            mxSetData (Ab_mx, Ab) ;
            GBMDUMP ("gb_export, remove Ab from memtable %p\n", Ab) ;
            GB_Global_memtable_remove (Ab) ; Ab = NULL ;
        }
        mxSetFieldByNumber (G, 0, 3, Ab_mx) ;
    }

    //--------------------------------------------------------------------------
    // return the built-in MATLAB struct containing the GrB_Matrix components
    //--------------------------------------------------------------------------

    return (G) ;
}

