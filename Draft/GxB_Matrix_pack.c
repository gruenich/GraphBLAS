// Methods, and an example.

GrB_Info GxB_Matrix_pack    // pack a matrix in any format
(
    GrB_Matrix A,               // matrix to pack from the Container
    GxB_Container Container,    // Container with contents to pack into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Vector_pack    // pack a Vector in any format
(
    GrB_Vector A,               // Vector to pack from the Container
    GxB_Container Container,    // Container with contents to pack into A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Matrix_unpack  // unpack a matrix in any format
(
    GrB_Matrix A,               // matrix to unpack into the Container
    GxB_Container Container,    // Container to hold the contents from A
    const GrB_Descriptor desc
) ;

GrB_Info GxB_Vector_unpack  // unpack a Vector in any format
(
    GrB_Vector A,               // Vector to unpack into the Container
    GxB_Container Container,    // Container to hold the contents from A
    const GrB_Descriptor desc
) ;

// Naming of the above 4 methods:  since they all use the GxB_Container object,
// perhaps a better naming scheme would be:
/*
    GxB_Container_pack_Matrix
    GxB_Container_pack_Vector
    GxB_Container_unpack_Matrix
    GxB_Container_unpack_Vector
*/

GrB_Info GxB_Container_new  // create a Container
(
    GxB_Container &Container
) ;

GrB_Info GxB_Container_free // free a Container
(
    GxB_Container &Container
) ;

//------------------------------------------------------------------------------

GrB_Info GxB_Container_unpack   // unpack a single GrB_Vector from a Container
(
    GrB_Vector v,       // Container->[phibx] to unpack
    void **vx,          // values
    uint64_t *vlen,     // length of the vector
    GrB_Type *vtype     // type of vector
    const GrB_Descriptor desc
) ;

    // On input, vx is a NULL pointer.  On output, it holds a C array of length
    // vlen, and of type vtype.  On output, the Container->vector has no
    // entries and has a new length of 0.

    // Naming:  should this perhaps be called:
    //      GxB_Container_unpack_contents?

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_unpack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of
    // v starts as zero, and is set to vlen when the pack is done.

//------------------------------------------------------------------------------

GrB_Info GxB_Container_pack     // pack a single GrB_Vector from a Container
(
    GrB_Vector v,       // Container->[phibx] to unpack
    void **vx,          // values
    uint64_t vlen,      // length of the vector
    GrB_Type vtype,     // type of vector
    const GrB_Descriptor desc
) ;

    // On input, vx a pointer to a C array of length vlen and type vtype.
    // On output, vx is NULL.  Its contents have been moved into the
    // Container->vector, which now has length vlen and type vtype.

    // Naming:  should this perhaps be called:
    //      GxB_Container_pack_contents?

    // This method can be used on any GrB_Vector v that is completely full with
    // all its entries present.  It is like my existing GxB_Vector_pack_Full,
    // but the latter leaves the length of v unchanged.  Here, the length of
    // v is set to zero when the pack is done.

//------------------------------------------------------------------------------
// Example
//------------------------------------------------------------------------------

// Usage: with a given GrB_Matrix A to unpack/pack, of size nrows-by-ncols,
// with nvals entries, of type atype.  The following will take O(1) time,
// and the only mallocs are in GxB_Container_new (which can be reused for
// an arbitrary number of  pack/unpack cycles).

GxB_Container_new (&Container) ;    // requires several O(1)-sized mallocs

for (as many times as you like)
{

    GxB_Matrix_unpack (A, Container, desc) ;
    // A is now 0-by-0 with nvals(A)=0.  Its type is unchanged.

    // All of the following is optional; if any item in the Container is not
    // needed by the user, it can be left as-is, and then it will be packed
    // back into A at the end.  (This is done for the Container->Y).

    // to extract numerical values from the Container:
    void *x = NULL ;
    uint64_t nvals = 0 ;
    GrB_Type atype = NULL ;
    GxB_Container_unpack (Container->x, &x, &nvals, &atype, desc) ;

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
            // The Container->Y matrix can be unpacked here as well,
            // if desired.  Its use is optional.
            GxB_Container_unpack (Container->h, &h, &plen, &htype, desc) ;
        case GxB_SPARSE :
            GxB_Container_unpack (Container->p, &p, &plen1, &ptype, desc) ;
            GxB_Container_unpack (Container->i, &i, &nvals, &itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Container_unpack (Container->b, &b, &nvals, &btype, desc) ;
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
            GxB_Container_pack (Container->h, &h, plen, htype, desc) ;
        case GxB_SPARSE :
            GxB_Container_unpack (Container->p, p, plen1, ptype, desc) ;
            GxB_Container_unpack (Container->i, i, nvals, itype, desc) ;
            break ;
        case GxB_BITMAP :
            GxB_Container_unpack (Container->b, b, nvals, btype, desc) ;
            break ;
    }
    GxB_Container_pack (Container->x, &x, nvals, atype, desc) ;

    // Now the C arrays p, h, i, b, and x are all NULL.  They are in the
    // Container->p,h,b,i,x GrB_Vectors.  Pack the non-opaque Container
    // back into A:

    GxB_Matrix_pack (A, Container, desc) ;
    // A is now back to its original state.  The Container->p,h,b,i,x
    // GrB_Vectors exist but all have length 0.

}

GxB_Container_free (&Container) ;    // does several O(1)-sized free's

