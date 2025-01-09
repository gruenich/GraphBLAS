function bench2
%BENCH2 benchmark kron

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

% FIXME: GB_mex_hack (5) not used by GraphBLAS; use GB_mex_control

rng ('default') ;

GB_mex_burble (1) ;
s = maxNumCompThreads (32) ;
save = nthreads_set (32)  ;

m = 300 ;
n = m*m ;
Cin.matrix = sparse (n, n) ;
Cin.sparsity = 1 ;
Cin.iso = true ;

A_rand = sprand (m, m, 0.5) ;
B_rand = sprand (m, m, 0.5) ;

for iso = 0:1
    clear A B

    fprintf ('\n============= iso %d\n', iso) ;
    A.matrix = A_rand ;
    B.matrix = B_rand ;
    if (iso)
        A.matrix = spones (A.matrix) ;
        B.matrix = spones (B.matrix) ;
    end
    A.iso = iso ;
    B.iso = iso ;

    op.opname = 'times' ;
    op.optype = 'double' ;

    hack_save = GB_mex_hack ;
    hack = hack_save ;
    clear C1

    for is_32 = 0:1
        hack (5) = is_32 ; GB_mex_hack (hack) ; % modify is_32 (0 or 1)
        C1 {is_32+1} = GB_mex_kron (Cin, [ ], [ ], op, A, B, [ ]) ;
        assert (C1 {is_32+1}.iso == iso) ;
    end
    assert (isequal (C1{1}.matrix, C1{2}.matrix)) ;
end

GB_mex_hack (hack_save) ;

fprintf ('\nbench2: all tests passed\n') ;

maxNumCompThreads (s) ;
nthreads_set (save)  ;
GrB.burble (0) ;

