//------------------------------------------------------------------------------
// GB_mex_extractTuples: extract all tuples from a matrix or vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_mex.h"

#define USAGE "[I,J,X] = GB_mex_extractTuples (A, xtype, itype)"

#define FREE_ALL                        \
{                                       \
    GrB_Matrix_free_(&A) ;              \
    GB_mx_put_global (true) ;           \
}

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    bool malloc_debug = GB_mx_get_global (true) ;
    GrB_Matrix A = NULL ;
    uint64_t nvals = 0 ;

    // check inputs
    if (nargout > 3 || nargin < 1 || nargin > 3)
    {
        mexErrMsgTxt ("Usage: " USAGE) ;
    }

    #define GET_DEEP_COPY ;
    #define FREE_DEEP_COPY ;

    // get A (shallow copy)
    A = GB_mx_mxArray_to_Matrix (pargin [0], "A input", false, true) ;
    if (A == NULL)
    {
        FREE_ALL ;
        mexErrMsgTxt ("A failed") ;
    }

    // get the number of entries in A
    GrB_Matrix_nvals (&nvals, A) ;

    // get the itype for I and J
    GrB_Type itype = GrB_UINT64 ;
    if (nargin > 2)
    {
        itype = GB_mx_string_to_Type (pargin [2], GrB_UINT64) ;
    }
    if (! (itype == GrB_UINT32 || itype == GrB_UINT64))
    {
        mexErrMsgTxt ("itype must be uint32 or uint64") ;
    }

    // create I
    pargout [0] = GB_mx_create_full (nvals, 1, itype) ;
    void *I_output = mxGetData (pargout [0]) ;

    // create J
    void *J_output = NULL ;
    if (nargout > 1)
    {
        pargout [1] = GB_mx_create_full (nvals, 1, itype) ;
        J_output = mxGetData (pargout [1]) ;
    }

    // create X
    GB_void *X = NULL ;
    GrB_Type xtype = GB_mx_string_to_Type (PARGIN (1), A->type) ;
    if (nargout > 2)
    {
        pargout [2] = GB_mx_create_full (nvals, 1, xtype) ;
        X = (GB_void *) mxGetData (pargout [2]) ;
    }

    // [I,J,X] = find (A)
    if (itype == GrB_UINT32)
    {
        uint32_t *I = (uint32_t *) I_output ;
        uint32_t *J = (uint32_t *) J_output ;

        if (GB_VECTOR_OK (A))
        {
            // test extract vector methods
            GrB_Vector v = (GrB_Vector) A ;
            switch (xtype->code)
            {
                case GB_BOOL_code   : METHOD (GxB_Vector_extractTuples_32_BOOL_  (I, (bool       *) X, &nvals, v)) ; break ;
                case GB_INT8_code   : METHOD (GxB_Vector_extractTuples_32_INT8_  (I, (int8_t     *) X, &nvals, v)) ; break ;
                case GB_UINT8_code  : METHOD (GxB_Vector_extractTuples_32_UINT8_ (I, (uint8_t    *) X, &nvals, v)) ; break ;
                case GB_INT16_code  : METHOD (GxB_Vector_extractTuples_32_INT16_ (I, (int16_t    *) X, &nvals, v)) ; break ;
                case GB_UINT16_code : METHOD (GxB_Vector_extractTuples_32_UINT16_(I, (uint16_t   *) X, &nvals, v)) ; break ;
                case GB_INT32_code  : METHOD (GxB_Vector_extractTuples_32_INT32_ (I, (int32_t    *) X, &nvals, v)) ; break ;
                case GB_UINT32_code : METHOD (GxB_Vector_extractTuples_32_UINT32_(I, (uint32_t   *) X, &nvals, v)) ; break ;
                case GB_INT64_code  : METHOD (GxB_Vector_extractTuples_32_INT64_ (I, (int64_t    *) X, &nvals, v)) ; break ;
                case GB_UINT64_code : METHOD (GxB_Vector_extractTuples_32_UINT64_(I, (uint64_t   *) X, &nvals, v)) ; break ;
                case GB_FP32_code   : METHOD (GxB_Vector_extractTuples_32_FP32_  (I, (float      *) X, &nvals, v)) ; break ;
                case GB_FP64_code   : METHOD (GxB_Vector_extractTuples_32_FP64_  (I, (double     *) X, &nvals, v)) ; break ;
                case GB_FC32_code   : METHOD (GxB_Vector_extractTuples_32_FC32_  (I, (GxB_FC32_t *) X, &nvals, v)) ; break ;
                case GB_FC64_code   : METHOD (GxB_Vector_extractTuples_32_FC64_  (I, (GxB_FC64_t *) X, &nvals, v)) ; break ;
                case GB_UDT_code    : METHOD (GxB_Vector_extractTuples_32_UDT_   (I, (void       *) X, &nvals, v)) ; break ;
                default             : FREE_ALL ; mexErrMsgTxt ("unsupported type") ;
            }
            if (J != NULL)
            {
                for (int64_t p = 0 ; p < nvals ; p++) J [p] = 0 ;
            }
        }
        else
        {
            switch (xtype->code)
            {
                case GB_BOOL_code   : METHOD (GxB_Matrix_extractTuples_32_BOOL_  (I, J, (bool       *) X, &nvals, A)) ; break ;
                case GB_INT8_code   : METHOD (GxB_Matrix_extractTuples_32_INT8_  (I, J, (int8_t     *) X, &nvals, A)) ; break ;
                case GB_UINT8_code  : METHOD (GxB_Matrix_extractTuples_32_UINT8_ (I, J, (uint8_t    *) X, &nvals, A)) ; break ;
                case GB_INT16_code  : METHOD (GxB_Matrix_extractTuples_32_INT16_ (I, J, (int16_t    *) X, &nvals, A)) ; break ;
                case GB_UINT16_code : METHOD (GxB_Matrix_extractTuples_32_UINT16_(I, J, (uint16_t   *) X, &nvals, A)) ; break ;
                case GB_INT32_code  : METHOD (GxB_Matrix_extractTuples_32_INT32_ (I, J, (int32_t    *) X, &nvals, A)) ; break ;
                case GB_UINT32_code : METHOD (GxB_Matrix_extractTuples_32_UINT32_(I, J, (uint32_t   *) X, &nvals, A)) ; break ;
                case GB_INT64_code  : METHOD (GxB_Matrix_extractTuples_32_INT64_ (I, J, (int64_t    *) X, &nvals, A)) ; break ;
                case GB_UINT64_code : METHOD (GxB_Matrix_extractTuples_32_UINT64_(I, J, (uint64_t   *) X, &nvals, A)) ; break ;
                case GB_FP32_code   : METHOD (GxB_Matrix_extractTuples_32_FP32_  (I, J, (float      *) X, &nvals, A)) ; break ;
                case GB_FP64_code   : METHOD (GxB_Matrix_extractTuples_32_FP64_  (I, J, (double     *) X, &nvals, A)) ; break ;
                case GB_FC32_code   : METHOD (GxB_Matrix_extractTuples_32_FC32_  (I, J, (GxB_FC32_t *) X, &nvals, A)) ; break ;
                case GB_FC64_code   : METHOD (GxB_Matrix_extractTuples_32_FC64_  (I, J, (GxB_FC64_t *) X, &nvals, A)) ; break ;
                case GB_UDT_code    : METHOD (GxB_Matrix_extractTuples_32_UDT_   (I, J, (void       *) X, &nvals, A)) ; break ;
                default             : FREE_ALL ; mexErrMsgTxt ("unsupported type") ;
            }
        }

    }
    else
    {
        uint64_t *I = (uint64_t *) I_output ;
        uint64_t *J = (uint64_t *) J_output ;

        if (GB_VECTOR_OK (A))
        {
            // test extract vector methods
            GrB_Vector v = (GrB_Vector) A ;
            switch (xtype->code)
            {
                case GB_BOOL_code   : METHOD (GrB_Vector_extractTuples_BOOL_  (I, (bool       *) X, &nvals, v)) ; break ;
                case GB_INT8_code   : METHOD (GrB_Vector_extractTuples_INT8_  (I, (int8_t     *) X, &nvals, v)) ; break ;
                case GB_UINT8_code  : METHOD (GrB_Vector_extractTuples_UINT8_ (I, (uint8_t    *) X, &nvals, v)) ; break ;
                case GB_INT16_code  : METHOD (GrB_Vector_extractTuples_INT16_ (I, (int16_t    *) X, &nvals, v)) ; break ;
                case GB_UINT16_code : METHOD (GrB_Vector_extractTuples_UINT16_(I, (uint16_t   *) X, &nvals, v)) ; break ;
                case GB_INT32_code  : METHOD (GrB_Vector_extractTuples_INT32_ (I, (int32_t    *) X, &nvals, v)) ; break ;
                case GB_UINT32_code : METHOD (GrB_Vector_extractTuples_UINT32_(I, (uint32_t   *) X, &nvals, v)) ; break ;
                case GB_INT64_code  : METHOD (GrB_Vector_extractTuples_INT64_ (I, (int64_t    *) X, &nvals, v)) ; break ;
                case GB_UINT64_code : METHOD (GrB_Vector_extractTuples_UINT64_(I, (uint64_t   *) X, &nvals, v)) ; break ;
                case GB_FP32_code   : METHOD (GrB_Vector_extractTuples_FP32_  (I, (float      *) X, &nvals, v)) ; break ;
                case GB_FP64_code   : METHOD (GrB_Vector_extractTuples_FP64_  (I, (double     *) X, &nvals, v)) ; break ;
                case GB_FC32_code   : METHOD (GxB_Vector_extractTuples_FC32_  (I, (GxB_FC32_t *) X, &nvals, v)) ; break ;
                case GB_FC64_code   : METHOD (GxB_Vector_extractTuples_FC64_  (I, (GxB_FC64_t *) X, &nvals, v)) ; break ;
                case GB_UDT_code    : METHOD (GrB_Vector_extractTuples_UDT_   (I, (void       *) X, &nvals, v)) ; break ;
                default             : FREE_ALL ; mexErrMsgTxt ("unsupported type") ;
            }
            if (J != NULL)
            {
                for (int64_t p = 0 ; p < nvals ; p++) J [p] = 0 ;
            }
        }
        else
        {
            switch (xtype->code)
            {
                case GB_BOOL_code   : METHOD (GrB_Matrix_extractTuples_BOOL_  (I, J, (bool       *) X, &nvals, A)) ; break ;
                case GB_INT8_code   : METHOD (GrB_Matrix_extractTuples_INT8_  (I, J, (int8_t     *) X, &nvals, A)) ; break ;
                case GB_UINT8_code  : METHOD (GrB_Matrix_extractTuples_UINT8_ (I, J, (uint8_t    *) X, &nvals, A)) ; break ;
                case GB_INT16_code  : METHOD (GrB_Matrix_extractTuples_INT16_ (I, J, (int16_t    *) X, &nvals, A)) ; break ;
                case GB_UINT16_code : METHOD (GrB_Matrix_extractTuples_UINT16_(I, J, (uint16_t   *) X, &nvals, A)) ; break ;
                case GB_INT32_code  : METHOD (GrB_Matrix_extractTuples_INT32_ (I, J, (int32_t    *) X, &nvals, A)) ; break ;
                case GB_UINT32_code : METHOD (GrB_Matrix_extractTuples_UINT32_(I, J, (uint32_t   *) X, &nvals, A)) ; break ;
                case GB_INT64_code  : METHOD (GrB_Matrix_extractTuples_INT64_ (I, J, (int64_t    *) X, &nvals, A)) ; break ;
                case GB_UINT64_code : METHOD (GrB_Matrix_extractTuples_UINT64_(I, J, (uint64_t   *) X, &nvals, A)) ; break ;
                case GB_FP32_code   : METHOD (GrB_Matrix_extractTuples_FP32_  (I, J, (float      *) X, &nvals, A)) ; break ;
                case GB_FP64_code   : METHOD (GrB_Matrix_extractTuples_FP64_  (I, J, (double     *) X, &nvals, A)) ; break ;
                case GB_FC32_code   : METHOD (GxB_Matrix_extractTuples_FC32_  (I, J, (GxB_FC32_t *) X, &nvals, A)) ; break ;
                case GB_FC64_code   : METHOD (GxB_Matrix_extractTuples_FC64_  (I, J, (GxB_FC64_t *) X, &nvals, A)) ; break ;
                case GB_UDT_code    : METHOD (GrB_Matrix_extractTuples_UDT_   (I, J, (void       *) X, &nvals, A)) ; break ;
                default             : FREE_ALL ; mexErrMsgTxt ("unsupported type") ;
            }
        }
    }

    FREE_ALL ;
}

