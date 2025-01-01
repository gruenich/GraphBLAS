//------------------------------------------------------------------------------
// GxB_Container: non-opaque, unpacked content of a GrB_Matrix and GrB_Vector
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

typedef struct GxB_Container_struct *GxB_Container ;

struct GxB_Container_struct
{

// A matrix can be held in one of 8 formats, each one consisting of a set of
// vectors.  The vector "names" are in the range 0 to A->vdim-1.  Each
// vector has length A->vlen.  These two values define the dimension of the
// matrix, where A is m-by-n.  The m and n dimenions are vlen and vdim for the
// CSC formats, and reversed for the CSR formats.

// Ap, Ai, Ax, Ah, and Ab are abbreviations for A->p, A->i, A->x, A->h, and
// A->b, respectively.

// For the sparse and hypersparse formats, Ap is an integer array of size
// A->plen+1, with Ap [0] always zero.  The matrix contains A->nvec sparse
// vectors, where A->nvec <= A->plen <= A->vdim.  The arrays Ai and Ax define
// the indices and values in each sparse vector.  The total number of entries
// in the matrix is Ap [nvec] <= GB_nnz_max (A).
// A->nvals is equal to Ap [nvec].

// For the bitmap and full sparsity structures, Ap and Ai are NULL.

// For both hypersparse and non-hypersparse matrices, if A->nvec_nonempty is
// computed, it is the number of vectors that contain at least one entry, where
// 0 <= A->nvec_nonempty <= A->nvec always holds.  If not computed,
// A->nvec_nonempty is equal to -1.

//------------------------------------------------------------------------------
// The 8 formats:  (hypersparse, sparse, bitmap, full) x (CSR or CSC)
//------------------------------------------------------------------------------

// --------------------------------------
// Full structure:
// --------------------------------------

    // Ah, Ap, Ai, and Ab are all NULL.
    // A->nvec == A->vdim.   A->plen is not needed (set to -1)

    // --------------------------------------
    // A->is_csc is true:  full CSC format
    // --------------------------------------

        // A is m-by-n: where A->vdim = n, and A->vlen = m

        // Column A(:,j) is held in Ax [p1:p2-1] where p1 = k*m, p2 = (k+1)*m.
        // A(i,j) at position p has row index i = p%m and value Ax [p]

    // --------------------------------------
    // A->is_csc is false:  full CSR format
    // --------------------------------------

        // A is m-by-n: where A->vdim = m, and A->vlen = n

        // Row A(i,:) is held in Ax [p1:p2-1] where p1 = k*n, p2 = (k+1)*n.
        // A(i,j) at position p has column index j = p%n and value Ax [p]

// --------------------------------------
// Bitmap structure:
// --------------------------------------

    // Ah, Ap, and Ai are NULL.  Ab is an int8_t array of size m*n.
    // A->nvec == A->vdim.   A->plen is not needed (set to -1)

    // The bitmap structure is identical to the full structure, except for the
    // addition of the bitmap array A->b.

    // --------------------------------------
    // A->is_csc is true:  bitmap CSC format
    // --------------------------------------

        // A is m-by-n: where A->vdim = n, and A->vlen = m

        // Column A(:,j) is held in Ax [p1:p2-1] where p1 = k*m, p2 = (k+1)*m.
        // A(i,j) at position p has row index i = p%m and value Ax [p].
        // The entry A(i,j) is present if Ab [p] == 1, and not present if
        // Ab [p] == 0.

    // --------------------------------------
    // A->is_csc is false:  bitmap CSR format
    // --------------------------------------

        // A is m-by-n: where A->vdim = m, and A->vlen = n

        // Row A(i,:) is held in Ax [p1:p2-1] where p1 = k*n, p2 = (k+1)*n.
        // A(i,j) at position p has column index j = p%n and value Ax [p]
        // The entry A(i,j) is present if Ab [p] == 1, and not present if
        // Ab [p] == 0.

// --------------------------------------
// Sparse structure:
// --------------------------------------

    // Ah and Ab are NULL
    // A->nvec == A->plen == A->vdim

    // --------------------------------------
    // A->is_csc is true:  sparse CSC format
    // --------------------------------------

        // Ap, Ai, and Ax store a sparse matrix in the a very similar style
        // as CSparse, as a collection of sparse column vectors.

        // Column A(:,j) is held in two parts: the row indices are in
        // Ai [Ap [j]...Ap [j+1]-1], and the numerical values are in the
        // same positions in Ax.

        // A is m-by-n: where A->vdim = n, and A->vlen = m

    // --------------------------------------
    // A->is_csc is false:  sparse CSR format
    // --------------------------------------

        // Ap, Ai, and Ax store a sparse matrix in CSR format, as a collection
        // of sparse row vectors.

        // Row A(i,:) is held in two parts: the column indices are in
        // Ai [Ap [i]...Ap [i+1]-1], and the numerical values are in the
        // same positions in Ax.

        // A is m-by-n: where A->vdim = m, and A->vlen = n

// --------------------------------------
// Hypersparse structure:
// --------------------------------------

    // Ab is NULL
    // Ah is non-NULL and has size A->plen; it is always kept sorted,
    // A->nvec <= A->plen <= A->vdim

    // --------------------------------------
    // A->is_csc is true: hypersparse CSC format
    // --------------------------------------

        // A is held as a set of A->nvec sparse column vectors, but not all
        // columns 0 to n-1 are present.

        // If column A(:,j) has any entries, then j = Ah [k] for some
        // k in the range 0 to A->nvec-1.

        // Column A(:,j) is held in two parts: the row indices are in Ai [Ap
        // [k]...Ap [k+1]-1], and the numerical values are in the same
        // positions in Ax.

        // A is m-by-n: where A->vdim = n, and A->vlen = m

    // --------------------------------------
    // A->is_csc is false: hypersparse CSR format
    // --------------------------------------

        // A is held as a set of A->nvec sparse row vectors, but not all
        // row 0 to m-1 are present.

        // If row A(i,:) has any entries, then i = Ah [k] for some
        // k in the range 0 to A->nvec-1.

        // Row A(i,:) is held in two parts: the column indices are in Ai
        // [Ap [k]...Ap [k+1]-1], and the numerical values are in the same
        // positions in Ax.

        // A is m-by-n: where A->vdim = n, and A->vlen = m

//------------------------------------------------------------------------------
// primary matrix content
//------------------------------------------------------------------------------

// 16 words of uint64_t:
uint64_t nrows ;            // # of rows of the matrix, or length of the vector
uint64_t ncols ;            // # of cols of the matrix; 1 for a vector
uint64_t nrows_nonempty ;   // the number of non-empty rows for CSR, CSC,
                            // HyperCSR, or HyperCSC matrices, or 
                            // UINT64_MAX if not known.
uint64_t ncols_nonempty ;   // the number of non-empty columns for CSR, CSC,
                            // HyperCSR, or Hyper CSC matrices.
                            // UINT64_MAX if not known; 1 for a vector
uint64_t nvals ;            // nvals(A)
uint64_t u64_future [11] ;  // for future expansion.

// 16 words of uint32_t:
int32_t format ;            // GxB_SPARSE, GxB_SPARSE, GxB_BITMAP, or GxB_FULL
int32_t orientation ;       // GrB_ROWMAJOR or GrB_COLMAJOR
int32_t i32_future [15] ;   // for future expansion.

// 16 GrB_Vector objects:

// The following GrB_Vectors are all dense.  The p, h, and i vectors can be
// of type GrB_UINT32 or GrB_UINT64.  The b vector is GrB_UINT8.  The x vector
// has the same type as the original GrB_Matrix or GrB_Vector.

GrB_Vector p ;  // pointers: vector of length plen+1 for CSR, CSC, HyperCSR,
                // and HyperCSC.  Length 0 for bitmap/full.  plen = nrows or
                // ncols if A is CSR/CSC respectively, or the # of rows/cols
                // present in a HyperCSR/HyperCSC matrix, respectively.
                // For CSR/CSC matrices, plen=1.

GrB_Vector h ;  // non-empty vector list for the hyperlist, of length plen,
                // for HyperCSR and HyperCSC.
                // Length 0 for CSR, CSC, bitmap, and full matrices.

GrB_Vector b ;  // bitmap: vector of length nrows*ncols for bitmap matrices.
                // Length 0 for all other formats.

GrB_Vector i ;  // indices: vector of length nvals for CSR, CSC, HyperCSR,
                // and HyperCSC formats.  Length 0 for bitmap and full.

GrB_Vector x ;  // values: length nvals, or length 1 if A is iso.

GrB_Vector vector_future [11] ;     // for future expansion

//------------------------------------------------------------------------------
// hashing the hypersparse list
//------------------------------------------------------------------------------

/* The matrix Y is a hashed inverse of the A->h hyperlist, for a hypersparse
    matrix A.  It allows for fast lookup of entries in Ah.  Given j, the goal
    is to find k so that j = Ah [k], or to report that j is not in Ah.  The
    matrix A->Y allows for a fast lookup to compute this, without using a
    binary search.

        anvec = A->nvec
        avdim = A->vdim
        Ah = A->h
        nhash is the size of the hash table Y, which is always a power of 2.
            Its size is determined by GB_hyper_hash_build.

    Then A->Y has dimension Y->vdim = nhash (one vector in Y for each hash
    bucket), and Y->vlen = avdim.  If Y is considered as held in column-format,
    then Y is avlen-by-nhash.  The row/col format of Y is not important.  Each
    of its vectors (nhash of them) corresponds to a single hash bucket, and
    each hash bucket can hold up to avdim entries (assuming worst-case
    collisions where all entries j land in the same hash bucket).  Y is always
    in sparse format; never full, bitmap, or hypersparse.  Its type is always
    GrB_INT64, and it is never iso-valued.  The # of entries in Y is exactly
    anvec.

    Let f(j) = GB_HASHF2(j,nhash-1) be the hash function for the value j.  Its
    value is in the range 0 to nhash-1, where nhash is always a power of 2.

    If j = Ah [k], then k = Y (j,f (j)).
    If j is not in the Ah hyperlist, then Y (j,f(j)) does not appear
    as an entry in Y.

    Ideally, if the hash function had no collisions, each vector in Y would
    have length 0 or 1, and k = Y (j,f(j)) would be O(1) time lookup.
    However, the load factor is normally in the range of 2 to 4, so ideally
    each bucket will contain about 4 entries on average, if the load factor
    is 4.

    A->Y is only computed when required, or if GrB_Matrix_wait (Y) is
    explicitly called.  Once computed, k can be found as follows:

        // This can be done once, and reused for many searches:
        int64_t nhash = A->Y->vdim ;    // # of buckets in the hash table
        int64_t hash_bits = nhash-1 ;
        uint64_t *Yp = A->Y->p ;        // pointers to each hash bucket
                                        // Yp has size nhash+1.
        int64_t *Yi = A->Y->i ;         // "row" indices j; Yi has size anvec.
        int64_t *Yx = A->Y->x ;         // values k; Yx has size anvec.

        // Given a value j to find in the list Ah: find the entry k =
        // Y(j,f(j)), if it exists, or k=-1 if j is not in the Ah
        // hyperlist.
        int64_t jhash = GB_HASHF2 (j, hash_bits) ;     // in range 0 to nhash-1
        int64_t k = -1 ;
        for (int64_t p = Yp [jhash] ; p < Yp [jhash+1] ; p++)
        {
            if (j == Yi [p])
            {
                k = Yx [p] ;        // k = Y (j,jhash) has been found
                break ;
            }
            // or this could be done instead:
            // k = (j == Yi [p]) ? Yx [p] : k ;  // break not needed.
        }

    The hyper_hash is based on the HashGraph method by Oded Green,
    ACM Trans. Parallel Computing, June 2021, https://doi.org/10.1145/3460872
*/

GrB_Matrix Y ;      // Y is a matrix that represents the inverse of A->h.
                    // It can only be non-NULL if A is hypersparse.  Not all
                    // hypersparse matrices need the A->Y matrix.  It is
                    // constructed whenever it is needed.

GrB_Vector matrix_future [15] ;     // for future expansion

//------------------------------------------------------------------------------
// iso and jumbled matrices
//------------------------------------------------------------------------------

// Entries that are present in a GraphBLAS matrix, vector, or scalar always
// have a value, and thus the C API of GraphBLAS does not have a structure-only
// data type, where the matrix, vector, or scalar consists only of its pattern,
// with no values assign to the entries in the matrix.  Such an object might be
// useful for representing unweighted graphs, but it would result in a
// mathematical mismatch with all other objects.  Operations between valued
// matrices and structure-only matrices would not be defined.

// Instead, the common practice is to assign all entries present in the matrix
// to be equal to a single value, typically 1 or true.  SuiteSparse:GraphBLAS
// exploits this typical practice by allowing for iso matrices, where all
// entries present have the same value, held as A->x [0].  The sparsity
// structure is kept, so in an iso matrix, A(i,j) is either equal to A->x [0],
// or not present in the sparsity pattern of A.

// If A is full, A->x is the only component present, and thus a full iso matrix
// takes only O(1) memory, regardless of its dimension.

bool iso ;              // true if all entries have the same value

bool jumbled ;          // true if the matrix may be jumbled.  bitmap and full
                        // matrices are never jumbled.

bool bool_future [14] ; // for future expansion

//------------------------------------------------------------------------------
// iterating through a matrix
//------------------------------------------------------------------------------

// The matrix can be held in 8 formats: (hypersparse, sparse, bitmap, full) x
// (CSR, CSC).  Each of these can also be iso.  The comments below
// assume A is in CSC format but the code works for both CSR and CSC.
// The type is assumed to be double, just for illustration.

#ifdef for_comments_only    // only so vim will add color to the code below:

// for reference:
#define GBI(Ai,p,avlen) ((Ai == NULL) ? ((p) % (avlen)) : Ai [p])
#define GBB(Ab,p)       ((Ab == NULL) ? 1 : Ab [p])
#define GBP(Ap,k,avlen) ((Ap == NULL) ? ((k) * (avlen)) : Ap [k])
#define GBH(Ah,k)       ((Ah == NULL) ? (k) : Ah [k])

    // A->vdim: the vector dimension of A (ncols(A))
    // A->nvec: # of vectors that appear in A.  For the hypersparse case,
    //          these are the number of column indices in Ah [0..nvec-1], since
    //          A is CSC.  For all cases, Ap [0...nvec] are the pointers.

    //--------------------
    // (1) full      // A->h, A->p, A->i, A->b are NULL, A->nvec == A->vdim

        int64_t vlen = A->vlen ;
        for (k = 0 ; k < A->nvec ; k++)
        {
            j = k ;
            // operate on column A(:,j)
            int64_t pA_start = k * vlen ;
            int64_t pA_end   = (k+1) * vlen ;
            for (p = pA_start ; p < pA_end ; p++)
            {
                // entry A(i,j) with row index i and value aij
                int64_t i = (p % vlen) ;
                double aij = Ax [A->iso ? 0 : p] ;
            }
        }

    //--------------------
    // (2) bitmap    // A->h, A->p, A->i are NULL, A->nvec == A->vdim

        int64_t vlen = A->vlen ;
        for (k = 0 ; k < A->nvec ; k++)
        {
            j = k ;
            // operate on column A(:,j)
            int64_t pA_start = k * vlen ;
            int64_t pA_end   = (k+1) * vlen ;
            for (p = pA_start ; p < pA_end ; p++)
            {
                if (Ab [p] != 0)
                {
                    // entry A(i,j) with row index i and value aij
                    int64_t i = (p % vlen) ;
                    double aij = Ax [A->iso ? 0 : p] ;
                }
                else
                {
                    // A(i,j) is not present
                }
            }
        }

    //--------------------
    // (3) sparse     // A->h is NULL, A->nvec == A->vdim

        for (k = 0 ; k < A->nvec ; k++)
        {
            j = k ;
            // operate on column A(:,j)
            int64_t pA_start = Ap [k]
            int64_t pA_end   = Ap [k+1] ;
            for (p = pA_start ; p < pA_end ; p++)
            {
                // entry A(i,j) with row index i and value aij
                int64_t i = Ai [p] ;
                double aij = Ax [A->iso ? 0 : p] ;
            }
        }

    //--------------------
    // (4) hypersparse  // A->h is non-NULL, A->nvec <= A->vdim

        for (k = 0 ; k < A->nvec ; k++)
        {
            j = A->h [k]
            // operate on column A(:,j)
            int64_t pA_start = Ap [k]
            int64_t pA_end   = Ap [k+1] ;
            for (p = pA_start ; p < pA_end ; p++)
            {
                // entry A(i,j) with row index i and value aij
                int64_t i = Ai [p] ;
                double aij = Ax [A->iso ? 0 : p] ;
            }
        }

    //--------------------
    // generic: for any matrix

        int64_t vlen = A->vlen ;
        for (k = 0 ; k < A->nvec ; k++)
        {
            j = GBH (Ah, k) ;
            // operate on column A(:,j)
            int64_t pA_start = GBP (Ap, k, vlen) ;
            int64_t pA_end   = GBP (Ap, k+1, vlen) ;
            for (p = pA_start ; p < pA_end ; p++)
            {
                if (!GBB (Ab, p)) continue ;
                // entry A(i,j) with row index i and value aij
                int64_t i = GBI (Ai, p, vlen) ;
                double aij = Ax [A->iso ? 0 : p] ;
            }
        }

#endif

} ;

