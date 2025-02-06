
GxB_load_Matrix_from_Container  // matrix <= container
GxB_load_Vector_from_Container  // vector <= container
GxB_Vector_load                 // vector <= raw C array
GxB_unload_Matrix_to_Container  // container <= matrix
GxB_unload_Vector_to_Container  // container <= vector
GxB_Vector_unload               // raw C array <= vector

GxB_Container_load_Matrix       // container <- matrix
GxB_Container_unload_Matrix     // container -> matrix
GxB_Container_load_Vector       // container <- vector
GxB_Container_unload_Vector     // container -> vector
GxB_Vector_load                 // vector <- raw C array
GxB_Vector_unload               // vector -> raw C array

GxB_Container_Matrix_load       // container <- matrix
GxB_Container_Matrix_unload     // container -> matrix
GxB_Container_Vector_load       // container <- vector
GxB_Container_Vector_unload     // container -> vector
GxB_Vector_load                 // vector <- raw C array
GxB_Vector_unload               // vector -> raw C array

GrB_Info GxB_load_Matrix_from_Container     // GrB_Matrix <- GxB_Container
(
    GrB_Matrix A,               // matrix to load from the Container.  On input,
                                // A is a matrix of any size or type; on output
                                // any prior size, type, or contents is freed
                                // and overwritten with the Container.
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc   // currently unused
) ;

GrB_Info GxB_load_Vector_from_Container     // GrB_Vector <- GxB_Container
(
    GrB_Vector V,               // GrB_Vector to load from the Container
    GxB_Container Container,    // Container with contents to load into A
    const GrB_Descriptor desc   // currently unused
) ;

GrB_Info GxB_unload_Matrix_into_Container   // GrB_Matrix -> GxB_Container
(
    GrB_Matrix A,               // matrix to unload into the Container
    GxB_Container Container,    // Container to hold the contents of A
    const GrB_Descriptor desc   // currently unused
) ;

GrB_Info GxB_unload_Vector_into_Container   // GrB_Vector -> GxB_Container
(
    GrB_Vector V,               // vector to unload into the Container
    GxB_Container Container,    // Container to hold the contents of V
    const GrB_Descriptor desc   // currently unused
) ;

GrB_Info GxB_Vector_load    // vector <- raw C array
(
    // input/output:
    GrB_Vector V,           // vector to load from the C array X
    void **X,               // numerical array to load into V
    // input:
    GrB_Type type,          // type of X
    uint64_t n,             // # of entries in X
    uint64_t X_size,        // size of X in bytes (at least n*(sizeof the type))
    bool read_only,         // if true, X is treated as read-only
    const GrB_Descriptor desc   // currently unused; for future expansion
) ;

GrB_Info GxB_Vector_unload  // vector -> raw C array
(
    // input/output:
    GrB_Vector V,           // vector to unload
    void **X,               // numerical array to unload from V
    // output:
    GrB_Type *type,         // type of X
    uint64_t *n,            // # of entries in X
    uint64_t *X_size,       // size of X in bytes (at least n*(sizeof the type))
    bool *read_only,        // if true, X is treated as read-only
    const GrB_Descriptor desc   // currently unused; for future expansion
) ;

