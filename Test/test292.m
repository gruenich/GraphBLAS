function test292
%TEST292 test GxB_Vector_build_Scalar_Vector with a very large vector

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

[nth chunk] = nthreads_get ;
nthreads_set (8) ;
GB_mex_burble (1) ;
n = 5e9 ;
X = ones (n, 1, 'int8') ;
s = int8 (3) ;
op.opname = 'plus' ;
op.optype = 'int8' ;
desc.rowindex_list = 'use_indices' ;
V = GB_mex_Vector_build (X, s, n, op, 'int8', desc) ;
assert (nnz (V.matrix) == n)
assert (size (V.matrix,1) == n) ;
assert (min (V.matrix) == 3) ;
assert (max (V.matrix) == 3) ;
nthreads_set (nth, chunk) ;
GB_mex_burble (0) ;
fprintf ('test292: all tests passed\n') ;

