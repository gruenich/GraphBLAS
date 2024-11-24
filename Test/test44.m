function test44(longtests)
%TEST44 test qsort

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
% SPDX-License-Identifier: Apache-2.0

fprintf ('\ntest44\n------------------------------------- qsort tests\n') ;

if (nargin < 1)
    longtests = 1 ;
end

nlist = [0 1 5 100 50e3 103e3 200e3 1e6 ] ;
if (longtests)
    nlist = [nlist 10e6 100e6] ;
end
save = maxNumCompThreads ;

[save_nthreads save_chunk] = nthreads_get ;
nthreads_max = feature_numcores ;
fprintf ('maxNumCompThreads: %d  feature_numcores: %d\n', save, nthreads_max) ;

rng ('default') ;

for n = nlist

fprintf ('\n========================== n %g million\n', n / 1e6) ;

fprintf ('\n----------------------- qsort 1b\n') ;
% qsort1b is not stable; it used only when I has unique values
I = uint64 (randperm (n))' ;
J = uint64 ((n/10)* rand (n,1)) ;
IJ = [I J] ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJout = sortrows (IJ, 1) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
[Iout, Jout] = GB_mex_qsort_1b (I, J) ;
t2 = toc ;
fprintf ('built-in: sortrows %g sec  qsort1b: %g speedup: %g\n', t1, t2, t1/t2);
assert (isequal ([Iout Jout], IJout))
clear Iout Jout IJout

fprintf ('\n----------------------- qsort 1: 32 bit\n') ;
I = uint32 ((n/10)* rand (n,1)) ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJout = sortrows (I) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
Iout = GB_mex_qsort_1 (I) ;
t2 = toc ;
assert (isequal (Iout, IJout)) ;
clear Iout IJout
fprintf ('built-in: sortrows %g sec  qsort1: %g speedup: %g\n', t1, t2, t1/t2) ;

fprintf ('\n----------------------- qsort 1: 64 bit\n') ;
I = uint64 (I) ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJout = sortrows (I) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
Iout = GB_mex_qsort_1 (I) ;
t2 = toc ;
assert (isequal (Iout, IJout)) ;
clear Iout IJout
fprintf ('built-in: sortrows %g sec  qsort1: %g speedup: %g\n', t1, t2, t1/t2) ;

if (n > 200e6)
    continue ;
end

fprintf ('\n----------------------- qsort 2: 32 bit\n') ;
I = uint32 ((n/10)* rand (n,1)) ;
J = uint32 (randperm (n))' ;
IJ = [I J] ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJout = sortrows (IJ) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
[Iout, Jout] = GB_mex_qsort_2 (I, J) ;
t2 = toc ;
assert (isequal ([Iout Jout], IJout)) ;
clear Iout Jout IJout
fprintf ('built-in: sortrows %g sec  qsort2: %g speedup: %g\n', t1, t2, t1/t2) ;

fprintf ('\n----------------------- qsort 2: 64 bit\n') ;
I = uint64 (I) ;
J = uint64 (J) ;
IJ = [I J] ;
clear tt
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJout = sortrows (IJ) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    tt (nthreads) = t ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
[Iout, Jout] = GB_mex_qsort_2 (I, J) ;
t2 = toc ;
assert (isequal ([Iout Jout], IJout)) ;
clear Iout Jout
fprintf ('built-in: sortrows %g sec  qsort2: %g speedup: %g\n', t1, t2, t1/t2) ;

fprintf ('\nmsort2:\n') ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    tic
    [Iout, Jout] = GB_mex_msort_2 (I, J, nthreads) ;
    tp = toc ;
    if (nthreads == 1)
        tp1 = tp ;
    end
    assert (isequal ([Iout Jout], IJout)) ;
    clear Iout Jout
    fprintf ('msort2: %3d: %10.4g ', nthreads, tp) ;
    fprintf ('speedup vs 1: %8.3f ', tp1 / tp) ;
    fprintf ('speedup vs built-in: %8.3f\n', tt (nthreads) / tp) ;
end
clear IJout

fprintf ('\n----------------------- qsort 3: 32 bit\n') ;
I = uint32 ((n/10)* rand (n,1)) ;
J = uint32 ((n/10)* rand (n,1)) ;
K = uint32 (randperm (n))' ;
IJK = [I J K] ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJKout = sortrows (IJK) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
[Iout, Jout, Kout] = GB_mex_qsort_3 (I, J, K) ;
t2 = toc ;
assert (isequal ([Iout Jout Kout], IJKout))
clear Iout Jout Kout
clear IJKout
fprintf ('built-in: sortrows %g sec  qsort3: %g speedup: %g\n', t1, t2, t1/t2) ;

fprintf ('\n----------------------- qsort 3: 64 bit\n') ;
I = uint64 (I) ;
J = uint64 (J) ;
K = uint64 (K) ;
IJK = [I J K] ;
clear tt
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    maxNumCompThreads (nthreads) ;
    tic
    IJKout = sortrows (IJK) ;
    t = toc ;
    fprintf ('builtin: %2d threads: time %g\n', nthreads, t) ;
    tt (nthreads) = t ;
    if (nthreads == 1)
        t1 = t ;
    end
end
tic
[Iout, Jout, Kout] = GB_mex_qsort_3 (I, J, K) ;
t2 = toc ;
assert (isequal ([Iout Jout Kout], IJKout))
clear Iout Jout Kout
fprintf ('built-in: sortrows %g sec  qsort3: %g speedup: %g\n', t1, t2, t1/t2);

fprintf ('\nmsort3:\n') ;
for nthreads = [1 2 4 8 16 20 32 40 48 64 128 256]
    if (nthreads > 2*nthreads_max)
        break ;
    end
    tic
    [Iout, Jout, Kout] = GB_mex_msort_3 (I, J, K, nthreads) ;
    tp = toc ;
    if (nthreads == 1)
        tp1 = tp ;
    end
    assert (isequal ([Iout Jout Kout], IJKout)) ;
    clear Iout Jout Kout
    fprintf ('msort3: %3d: %10.4g ', nthreads, tp) ;
    fprintf ('speedup vs 1: %8.3f ', tp1 / tp) ;
    fprintf ('speedup vs built-in: %8.3f\n', tt (nthreads) / tp) ;
end
clear IJKout

end

fprintf ('\ntest44: all tests passed\n') ;
nthreads_set (save_nthreads, save_chunk) ;

maxNumCompThreads (save) ;

