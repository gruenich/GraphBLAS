

// existing:
    GrB_Matrix_build_TYPE (GrB_Matrix A,
        uint64_t *I,
        uint64_t *J,
        void *X, 
        uint64_t nvals
        GrB_BinaryOp dup)

// add:
    GxB_Matrix_build (GrB_Matrix A,
        GrB_Vector I,
        GrB_Vector J,
        GrB_Vector X, 
        GrB_BinaryOp dup,
        desc)

//  I, J, X: have 2 intrepations:
   indices of I and J, values

   GrB_Vector_extractTuples (I, X, nvals, v)
    1: list I of integers, monotonic increasing (no rep), length nvals
    2: list X of values,

    for I, use (1), indices     desc: 1 or 2
    for J, use (2), values      desc: 1 or 2
    for X,                      desc: 1 or 2

    nvals(I) == nvals(J) == nvals(X)

    GrB_assign (C, Mask, accum, A, I, ni, J, nj, desc)
    GrB_extract(C, Mask, accum, A, I, ni, J, nj, desc)


// existing:
GrB_Info GrB_Matrix_assign          // C<Mask>(I,J) = accum (C(I,J),A)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix Mask,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C(I,J),T)
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Index *I_,            // row indices
    GrB_Index ni,                   // number of row indices
    const GrB_Index *J,             // column indices
    GrB_Index nj,                   // number of column indices
    const GrB_Descriptor desc       // descriptor for C, Mask, and A
) ;

// new:
GrB_Info GxB_Matrix_assign          // C<Mask>(I,J) = accum (C(I,J),A)
(
    GrB_Matrix C,                   // input/output matrix for results
    const GrB_Matrix Mask,          // optional mask for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for Z=accum(C(I,J),T)
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Vector I,             // row indices
    const GrB_Vector J,             // column indices
    const GrB_Descriptor desc       // descriptor for C, Mask, and A
) ;


//  GrB_Info GrB_Matrix_extractTuples           // [I,J,X] = find (A)
//  (
//      uint64_t*I,             // array for returning row indices of tuples
//      uint64_ *J,             // array for returning col indices of tuples
//      <type> *X,              // array for returning values of tuples
//      GrB_Index *nvals,       // I,J,X size on input; # tuples on output
//      const GrB_Matrix A      // matrix to extract tuples from
//  ) ;

    GxB_Matrix_extractTuples (
        GrB_Vector I,
        GrB_Vector J,
        GrB_Vector X,
        const GrB_Matrix A)

I, J, X are dense vectors of length nvals(A), of the correct type
