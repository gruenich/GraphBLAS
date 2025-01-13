function bench1
% BENCH1 benchmark GrB_build and GrB_extractTuples

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

% FIXME: does not use 32-bits

GB_mex_burble (1) ;
s = maxNumCompThreads (32) ;
save = nthreads_set (32)  ;

fprintf ('\nGrB.random:\n') ;
rng ('default') ;
n = 20e6 ;
m = n ;
nz = 300e6 ;
d = nz / n^2 ;
tic
% A = sprandn (n, n, d) ;
G = GrB.random (m, n, d) ;
toc
Cin = sparse (n,n) ;

check_results = false ;

fprintf ('\nconvert to MATLAB:\n') ;
tic
A = double (G) ;
toc

fprintf ('\nfind:\n') ;
tic
[I,J,X] = find (A) ;
toc
nz = length (I) ;
fprintf ('nz: %g million\n', nz/1e6) ;

for trial = 1:2

    if (trial == 2)
        fprintf ('\n---------------- randomize\n') ;
        k = randperm (nz) ;
        I = I (k) ;
        J = J (k) ;
        X = X (k) ;
    end

    fprintf ('\nsparse:\n') ;
    tic
    A1 = sparse (I, J, X, m, n) ;
    toc

    fprintf ('\nfind:\n') ;
    tic
    [IM,JM] = find (A1) ;
    toc

    if (check_results)
        fprintf ('sort rows:\n') ;
        tic
        [~,p1] = sortrows ([IM JM]) ;
        IM = IM (p1) ;
        JM = JM (p1) ;
        toc
    end

    fprintf ('\nGrB build:\n') ;
    I0 = uint64 (I) - 1 ;
    J0 = uint64 (J) - 1;
    tic
    A2 = GB_mex_Matrix_build (I0, J0, X, m, n, [ ]) ;
    toc
    assert (norm (A1 - A2.matrix, 1) < 1e-12)

    fprintf ('\nGrB extractTuples (pattern only):\n') ;
    tic
    [I64, J64] = GB_mex_extractTuples (A2) ;
    toc

    if (check_results)
        fprintf ('sort rows:\n') ;
        tic
        [~,p1] = sortrows ([I64 J64]) ;
        I64 = double (I64 (p1) + 1) ;
        J64 = double (J64 (p1) + 1) ;
        assert (isequal (IM, I64)) ;
        assert (isequal (JM, J64)) ;
        toc
    end

    fprintf ('\nGrB build 32:\n') ;
    I0 = uint32 (I0) ;
    J0 = uint32 (J0) ;
    tic
    A2 = GB_mex_Matrix_build (I0, J0, X, m, n, [ ]) ;
    toc
    assert (norm (A1 - A2.matrix, 1) < 1e-12)

    fprintf ('\nGrB extractTuples (32-bit pattern only):\n') ;
    tic
    [I32, J32] = GB_mex_extractTuples (A2, 'double', 'uint32') ;
    toc

    if (check_results)
        fprintf ('sort rows:\n') ;
        tic
        [~,p1] = sortrows ([I32 J32]) ;
        I32 = double (I32 (p1) + 1) ;
        J32 = double (J32 (p1) + 1) ;
        assert (isequal (IM, I32)) ;
        assert (isequal (JM, J32)) ;
        toc
    end
end

maxNumCompThreads (s) ;
nthreads_set (save)  ;
GB_mex_burble (0) ;

