//------------------------------------------------------------------------------
// GxB_Matrix_import_CSR: import a matrix in CSR format
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

#include "GB_export.h"

GrB_Info GxB_Matrix_import_CSR      // import a CSR matrix
(
    GrB_Matrix *A,          // handle of matrix to create
    GrB_Type type,          // type of matrix to create
    GrB_Index nrows,        // matrix dimension is nrows-by-ncols
    GrB_Index ncols,
    GrB_Index nvals,        // number of entries in the matrix
    // CSR format:
    int64_t nonempty,       // number of rows with at least one entry:
                            // either < 0 if not known, or >= 0 if exact
    GrB_Index **Ap,         // row "pointers", size nrows+1
    GrB_Index **Aj,         // column indices, size nvals
    void      **Ax,         // values, size nvals
    const GrB_Descriptor desc       // descriptor for # of threads to use
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Matrix_import_CSR (&A, type, nrows, ncols, nvals,"
        " nonempty, &Ap, &Aj, &Ax, desc)") ;
    GB_BURBLE_START ("GxB_Matrix_import_CSR") ;
    GB_IMPORT_CHECK ;

    GB_RETURN_IF_NULL (Ap) ;
    if (nvals > 0)
    { 
        GB_RETURN_IF_NULL (Aj) ;
        GB_RETURN_IF_NULL (Ax) ;
    }

    //--------------------------------------------------------------------------
    // import the matrix
    //--------------------------------------------------------------------------

    // allocate just the header of the matrix, not the content
    info = GB_new (A, // sparse, new header
        type, ncols, nrows, GB_Ap_null, false,
        GxB_SPARSE, GB_Global_hyper_switch_get ( ), 0, Context) ;
    if (info != GrB_SUCCESS)
    { 
        // out of memory for matrix header (size O(1))
        ASSERT (*A == NULL) ;
        return (info) ;
    }

    // transplant the user's content into the matrix
    (*A)->h = NULL ;
    (*A)->p = (int64_t *) (*Ap) ;
    (*Ap) = NULL ;
    (*A)->nzmax = nvals ;
    (*A)->plen = nrows ;
    (*A)->nvec = nrows ;
    (*A)->magic = GB_MAGIC ;

    if (nvals == 0)
    { 
        // free the user input Aj and Ax arrays, if they exist
        if (Aj != NULL) GB_FREE (*Aj) ;
        if (Ax != NULL) GB_FREE (*Ax) ;
    }
    else
    { 
        // transplant Aj and Ax into the matrix
        (*A)->i = (int64_t *) (*Aj) ;
        (*A)->x = (*Ax) ;
        (*Aj) = NULL ;
        (*Ax) = NULL ;
    }

    // < 0:  compute nvec_nonempty when needed
    // >= 0: nvec_nonempty must be exact
    (*A)->nvec_nonempty = (nonempty < 0) ? (-1) : nonempty ;

    // the matrix may be 0-by-0 and thus considered a full matrix
    if (GB_IS_FULL (*A))
    {
        (*A)->plen = -1 ;
        (*A)->nvec_nonempty = (nrows == 0) ? 0 : ncols ;
    }

    //--------------------------------------------------------------------------
    // import is successful
    //--------------------------------------------------------------------------

    ASSERT (*Ap == NULL) ;
    ASSERT (*Aj == NULL) ;
    ASSERT (*Ax == NULL) ;
    ASSERT_MATRIX_OK ((*A), "A CSR imported", GB0) ;
    GB_BURBLE_END ;
    return (GrB_SUCCESS) ;
}

