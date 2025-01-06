function gbtest20b
%GBTEST20B test bandwidth, isdiag, ceil, floor, round, fix

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

clear all
clear mex
gbmake

fprintf ('\nLast one:\n') ;
n = 2^60 ;
G = GrB (n, n) ;
G (n,1) = 1
GrB.burble (1) ;
[lo, hi] = bandwidth (G)
assert (lo == int64 (2^60) - 1)

fprintf ('\ngbtest20: all tests passed\n') ;

