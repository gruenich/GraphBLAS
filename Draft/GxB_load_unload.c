// Methods, and an example.  These methods are entirely new and do not use my
// existing "pack/unpack" naming scheme.  They all are guaranteed to take O(1)
// time and space.

GrB_Info GxB_load_Matrix_from_Container     // GrB_Matrix <- GxB_Container
(
    GrB_Matrix A,               // matrix to load from the Container.  On input,
                                // A is a matrix of any size or type; on output
                                // any prior size, type, or contents is freed
                                // and overwritten with the Container.
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_load_Vector_from_Container     // GrB_Vector <- GxB_Container
(
    GrB_Vector A,               // GrB_Vector to load from the Container
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_unload_Matrix_into_Container   // GrB_Matrix -> GxB_Container
(
    GrB_Matrix A,               // matrix to unload into the Container
    GxB_Container Container,    // Container to hold the contents of A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_unload_Vector_into_Container   // GrB_Vector -> GxB_Container
(
    GrB_Vector A,               // GrB_Vector to unload into the Container
    GxB_Container Container,    // Container to hold the contents of A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Container_new  // create a Container
(
    GxB_Container &Container
) ;

GrB_Info GxB_Container_free // free a Container
(
    GxB_Container &Container
) ;

//------------------------------------------------------------------------------

GrB_Info GxB_unload_Vector      // dense GrB_Vector -> (C array, length, type)
(
    GrB_Vector v,       // vector to unload into the the C array
    void **values,      // C array with the values of v
    uint64_t *n,        // length of the vector v
    GrB_Type *type      // type of vector v
    const GrB_Descriptor desc
) ;

    // On input, the vector v must dense.  On output, its length is reduced to
    // 0 and it contains no entries.

    // On input, values is NULL.  On output, it points to a C array of
    // length n whose type is given by the GrB_Type &type parameter.

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_unpack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of v
    // starts as zero, and is set to n when the "unload" is done.

//------------------------------------------------------------------------------

GrB_Info GxB_load_Vector        // (C array, length, type) -> dense GrB_Vector
(
    // input/output:
    GrB_Vector v,       // vector to load from the C array
    void **values,      // C array with the values of v
    uint64_t n,         // length of the vector v
    GrB_Type type,      // type of vector v
    const GrB_Descriptor desc
) ;

    // On input, values is a C array of length n, with a type that corresponds
    // to the GrB_Type type parameter.  On output, values is NULL.  Its
    // contents have been moved into the GrB_Vector v, which now has length n
    // and type type.

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_pack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of
    // v starts as zero (or any value) but is set to n when the "load" is done.

//------------------------------------------------------------------------------
// Example
//------------------------------------------------------------------------------

// Usage: with a given GrB_Matrix A to unload/load, of size nrows-by-ncols,
// with nvals entries, of type xtype.  The following will take O(1) time,
// and the only mallocs are in GxB_Container_new (which can be reused for
// an arbitrary number of load/unload cycles), and the only frees are in
// GxB_Container_free.

GxB_Container_new (&Container) ;    // requires several O(1)-sized mallocs

for (as many times as you like)
{

    GxB_unload_Matrix_into_Container (A, Container, desc) ;
    // A is now 0-by-0 with nvals(A)=0.  Its type is unchanged.

    // All of the following is optional; if any item in the Container is not
    // needed by the user, it can be left as-is, and then it will be put
    // back into A at the end.  (This is done for the Container->Y).

    // to extract numerical values from the Container:
    void *x = NULL ;
    uint64_t nvals = 0, nheld = 0 ;
    GrB_Type xtype = NULL ;
    GxB_Vector_unload (Container->x, &x, &nheld, &xtype, desc) ;

    // The C array x now has size nheld and contains the values of the original
    // GrB_Matrix A, with type xtype being the original type of the matrix A.
    // The Container->x GrB_Vector still exists but it now has length 0.

    // to extract the sparsity pattern from the Container:
    GrB_Type ptype = NULL, htype = NULL, itype = NULL, btype = NULL ;
    void *p = NULL, *h = NULL, *i = NULL, *b = NULL ;
    uint64_t plen = 0, plen1 = 0, nheld = 0 ;

    switch (Container->format)
    {
        case GxB_HYPERSPARSE :
            // The Container->Y matrix can be unloaded here as well,
            // if desired.  Its use is optional.
            GxB_Vector_unload (Container->h, &h, &plen, &htype, desc) ;
        case GxB_SPARSE :
            GxB_Vector_unload (Container->p, &p, &plen1, &ptype, desc) ;
            GxB_Vector_unload (Container->i, &i, &nvals, &itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Vector_unload (Container->b, &b, &nheld, &btype, desc) ;
            break ;
    }

    // Now the C arrays (p, h, i, b, and x) are all populated and owned by the
    // user application.  They can be modified here, if desired.
    // ...

    // to load them back into A, first load them into the Container->[phbix]
    // vectors:
    switch (Container->format)
    {
        case GxB_HYPERSPARSE :
            // The Container->Y matrix can be loaded here as well,
            // if desired.  Its use is optional.
            GxB_Vector_load (Container->h, &h, plen, htype, desc) ;
        case GxB_SPARSE :
            GxB_Vector_load (Container->p, &p, plen1, ptype, desc) ;
            GxB_Vector_load (Container->i, &i, nvals, itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Vector_load (Container->b, &b, nheld, btype, desc) ;
            break ;
    }
    GxB_Vector_load (Container->x, &x, nheld, xtype, desc) ;

    // Now the C arrays p, h, i, b, and x are all NULL.  They are in the
    // Container->p,h,b,i,x GrB_Vectors.  Load A from the non-opaque Container:

    GxB_load_Matrix_from_Container (A, Container, desc) ;
    // A is now back to its original state.  The Container and its p,h,b,i,x
    // GrB_Vectors exist but its vectors all have length 0.

}

GxB_Container_free (&Container) ;    // does several O(1)-sized free's

//------------------------------------------------------------------------------
// additional options:

// Support for COO / Tuple formats:

//      The Container method could be extended to the COO / Tuples format.  It
//      would be like GrB_Matrix_build when moving the tuples to a matrix, but
//      the method would be faster than GrB_Matrix_build /
//      GrB_Matrix_extractTuples.  The row indices, column indices, and values
//      in the Container could be moved into the matrix, saving time and space.
//      I already have this capability in my internal methods but there is no
//      user interface for it.

//      The Container could include a binary operator, which would be used to
//      combine duplicate entries.

//      The COO -> Container -> GrB_Matrix construction would not take O(1)
//      time and space, but it would be faster and take less memory than
//      GrB_Matrix_build.

//      I think this option would be important for the SparseBLAS, to allow for
//      fast load/unload of COO matrices into/from a GrB_Matrix or GrB_Vector.

// Support for user-owned arrays:

//      The SparseBLAS is considering a method where the user-owned, read-only
//      arrays are loaded into a Matrix object.  These would not be freed if
//      the GrB_Matrix is freed, and GraphBLAS must not modify them.  I have
//      an feature where I can handle this, where each p,h,i,b,x,Y component
//      is tagged as "shallow".

//      Loading a GrB_Matrix from a Container would need to have flags for
//      each of these components, to indicate if any are "shallow".

