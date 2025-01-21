function test290
%TEST290 test bitmap_subref on a large matrix

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

n = 3e9 ;
A = ones (n,2, 'int8') ;
A (1:2, 1:2) = [2 3 ; 4 5] ;
I = uint64 (0:1) ;
C = GB_mex_subref_symbolic (A, I, I) ;
S = [ 0   3000000000
      1   3000000001 ] ;
S = uint64 (S) ;
assert (isequal (C.matrix, S)) ;

fprintf ('test290: all tests passed\n') ;

