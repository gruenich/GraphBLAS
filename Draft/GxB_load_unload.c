// Methods, and an example.  These methods are entirely new and do not use my
// existing "pack/unpack" naming scheme.

GrB_Info GxB_Container_to_Matrix // load a matrix in any format from Container
(
    // output:
    GrB_Matrix A,               // matrix to load from the Container
    // input:
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Container_to_Vector // load a Vector in any format from Container
(
    // output:
    GrB_Vector A,               // Vector to load from the Container
    // input:
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Matrix_to_Container  // unload a matrix in any format to Container
(
    // output:
    GxB_Container Container,    // Container to hold the contents from A
    // input:
    GrB_Matrix A,               // matrix to unload into the Container
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Vector_to_Container  // unload a Vector in any format to Container
(
    // output:
    GxB_Container Container,    // Container to hold the contents from A
    // input:
    GrB_Vector A,               // Vector to unload into the Container
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

GrB_Info GxB_Container_unload   // unload a single GrB_Vector from a Container
(
    // input/output:
    GrB_Vector v,       // Container->[phibx] to unload
    void **vx,          // values
    uint64_t *vlen,     // length of the vector
    GrB_Type *vtype     // type of vector
    const GrB_Descriptor desc
) ;

    // On input, vx is a NULL pointer.  On output, it holds a C array of length
    // vlen, and of type vtype.  On output, the Container->vector has no
    // entries and has a new length of 0.

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_unpack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of
    // v starts as zero, and is set to vlen when the "unload" is done.

//------------------------------------------------------------------------------

GrB_Info GxB_Container_load     // load a single GrB_Vector from a Container
(
    // input/output:
    GrB_Vector v,       // Container->[phibx] to load
    void **vx,          // values
    uint64_t vlen,      // length of the vector
    GrB_Type vtype,     // type of vector
    const GrB_Descriptor desc
) ;

    // On input, vx a pointer to a C array of length vlen and type vtype.
    // On output, vx is NULL.  Its contents have been moved into the
    // Container->vector, which now has length vlen and type vtype.

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_pack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of
    // v is set to zero when the "load" is done.

//------------------------------------------------------------------------------
// Example
//------------------------------------------------------------------------------

// Usage: with a given GrB_Matrix A to unload/load, of size nrows-by-ncols,
// with nvals entries, of type atype.  The following will take O(1) time,
// and the only mallocs are in GxB_Container_new (which can be reused for
// an arbitrary number of  load/unload cycles).

GxB_Container_new (&Container) ;    // requires several O(1)-sized mallocs

for (as many times as you like)
{

    GxB_Matrix_to_Container (A, Container, desc) ;
    // A is now 0-by-0 with nvals(A)=0.  Its type is unchanged.

    // All of the following is optional; if any item in the Container is not
    // needed by the user, it can be left as-is, and then it will be put
    // back into A at the end.  (This is done for the Container->Y).

    // to extract numerical values from the Container:
    void *x = NULL ;
    uint64_t nvals = 0 ;
    GrB_Type atype = NULL ;
    GxB_Container_unload (Container->x, &x, &nvals, &atype, desc) ;

    // The C array x now has size nvals of the original A, with type atype
    // being the original type of the matrix A.  The Container->x GrB_Vector
    // still exists but it now has length 0.

    // to extract the sparsity pattern from the Container:
    GrB_Type ptype = NULL, htype = NULL, itype = NULL, btype = NULL ;
    void *p = NULL, *h = NULL, *i = NULL, *b = NULL ;
    uint64_t plen = 0, plen1 = 0 ;

    switch (Container->format)
    {
        case GxB_HYPERSPARSE :
            // The Container->Y matrix can be unloaded here as well,
            // if desired.  Its use is optional.
            GxB_Container_unload (Container->h, &h, &plen, &htype, desc) ;
        case GxB_SPARSE :
            GxB_Container_unload (Container->p, &p, &plen1, &ptype, desc) ;
            GxB_Container_unload (Container->i, &i, &nvals, &itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Container_unload (Container->b, &b, &nvals, &btype, desc) ;
            break ;
    }

    // Now the C arrays (p, h, i, b, and x) are all populated and owned by the
    // user application.  They can be modified here, if desired.
    // ...

    // to pack them back into A, first pack them into the Container->[phbix]
    // vectors:
    switch (Container->format)
    {
        case GxB_HYPERSPARSE :
            // The Container->Y matrix can be packed here as well,
            // if desired.  Its use is optional.
            GxB_Container_load (Container->h, &h, plen, htype, desc) ;
        case GxB_SPARSE :
            GxB_Container_load (Container->p, p, plen1, ptype, desc) ;
            GxB_Container_load (Container->i, i, nvals, itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Container_load (Container->b, b, nvals, btype, desc) ;
            break ;
    }
    GxB_Container_load (Container->x, &x, nvals, atype, desc) ;

    // Now the C arrays p, h, i, b, and x are all NULL.  They are in the
    // Container->p,h,b,i,x GrB_Vectors.  Pack the non-opaque Container
    // back into A:

    GxB_Container_to_Matrix (A, Container, desc) ;
    // A is now back to its original state.  The Container->p,h,b,i,x
    // GrB_Vectors exist but all have length 0.

}

GxB_Container_free (&Container) ;    // does several O(1)-sized free's

//------------------------------------------------------------------------------
// additional options:

// The Container method could be extended to the COO / Tuples format.  It would
// be like GrB_Matrix_build when moving the tuples to a matrix, but the method
// would be faster than GrB_Matrix_build / GrB_Matrix_extractTuples.  The
// row indices, column indices, and values in the Container could be moved
// into the matrix, saving time and space.  I already have this capability
// in my internal methods but there is no user interface for it.

// The container could include a binary operator, which would be used to
// combine duplicate entries.


