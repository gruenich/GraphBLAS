function Cout = apply (varargin)
%GB.APPLY apply a unary operator to a matrix.
%
% Usage:
%
%   Cout = gb.apply (op, A, desc)
%   Cout = gb.apply (Cin, accum, op, A, desc)
%   Cout = gb.apply (Cin, M, op, A, desc)
%   Cout = gb.apply (Cin, M, accum, op, A, desc)
%
% gb.apply applies a unary operator to the entries in the input matrix A,
% which may be a GraphBLAS or MATLAB matrix (sparse or full).  See 'help
% gb.unopinfo' for a list of available unary operators.  Cout is returned
% as a GraphBLAS matrix, by default; see 'help gb/descriptorinfo' for
% more options.
%
% The op and A arguments are required.
%
% accum: a binary operator to accumulate the results.
%
% Cin, and the mask matrix M, and the accum operator are optional.  If
% either accum or M is present, then Cin is a required input. If desc.in0
% is 'transpose' then A is transposed before applying the operator, as
% C<M> = accum (C, f(A')) where f(...) is the unary operator.
%
% See also spfun.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

[args, is_gb] = gb_get_args (varargin {:}) ;
if (is_gb)
    Cout = gb (gbapply (args {:})) ;
else
    Cout = gbapply (args {:}) ;
end

