function gbtest129
%GBTEST129 test GrB.timing

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

help GrB.timing ;
t = GrB.timing ;
t = GrB.timing (0) ;

fprintf ('\ngbtest129: all tests passed\n') ;
