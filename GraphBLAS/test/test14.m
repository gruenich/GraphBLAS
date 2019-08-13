function test14
%TEST14 test gb.gbkron

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

rng ('default') ;
A = sparse (rand (2,3)) ;
B = sparse (rand (4,8)) ;

C = kron (A,B) ;

G = gb.gbkron ('*', A, B) ;
err = norm (C-sparse(G), 1)
assert (err < 1e-12)

d.kind = 'sparse' ;
d.in0 = 'transpose' ;

G = gb.gbkron ('*', A, B, d) ;
C = kron (A', B) ;
err = norm (C-sparse(G), 1)
assert (err < 1e-12)

d.kind = 'gb' ;
G = gb.gbkron ('*', A, B, d) ;
G = sparse (G) ;
err = norm (C-G, 1)

E = sparse (rand (8,24)) ;
C = E + kron (A,B) ;
G = gb.gbkron (E, '+', '*', A, B) ; 
C-sparse(G)

fprintf ('test14: all tests passed\n') ;

