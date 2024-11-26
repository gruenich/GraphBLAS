function test287

GrB.burble (1) ;
s = maxNumCompThreads (32)
save = nthreads_set (32) 

fprintf ('\nsprand:\n') ;
n = 20e6 ;
m = n ;
nz = 400e6 ;
d = nz / n^2 ;
tic
A = sprandn (n, n, d) ;
toc
Cin = sparse (n,n) ;

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

    fprintf ('\nGrB build:\n') ;
    I0 = uint64 (I) - 1 ;
    J0 = uint64 (J) - 1;
    tic
    A2 = GB_mex_Matrix_build (I0, J0, X, m, n, [ ]) ;
    toc
    assert (norm (A1 - A2.matrix, 1) < 1e-12)

    fprintf ('\nGrB build 32:\n') ;
    I0 = uint32 (I0) ;
    J0 = uint32 (J0) ;
    tic
    A2 = GB_mex_Matrix_build (I0, J0, X, m, n, [ ]) ;
    toc
    assert (norm (A1 - A2.matrix, 1) < 1e-12)

end

maxNumCompThreads (s)
nthreads_set (save) 
GrB.burble (0) ;
