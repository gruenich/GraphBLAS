//------------------------------------------------------------------------------
// GB_jitifyer.c: CPU jitifyer
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"
#include "GB_stringify.h"
#include "GB_config.h"
#include "GB_zstd.h"
#include "GB_JITpackage.h"

//------------------------------------------------------------------------------
// determine if the JIT is enabled at compile-time
//------------------------------------------------------------------------------

#ifndef NJIT
// FIXME: dlfcn.h only exists on Linux/Unix/Mac; need to port to Windows
#include <dlfcn.h>
// FIXME: for Windows: need to use _mkdir(name) instead mkdir(name,mode)
#include <sys/stat.h>
#include <errno.h>
#else
// FIXME: for testing; force a compiler error if the code attempts to use
// dlopen, dlclose, dlsym, or mkdir (need to port to Windows)
#define dlopen garbage_dlopen
#define dlclose garbage_dlclose
#define dlsym garbage_dlsym
#define mkdir garbage_mkdir
#endif

typedef GB_JIT_KERNEL_USER_OP_PROTO ((*GB_user_op_f)) ;
typedef GB_JIT_KERNEL_USER_TYPE_PROTO ((*GB_user_type_f)) ;

//------------------------------------------------------------------------------
// GB_jitifyer_hash_table: a hash table of jitifyer entries
//------------------------------------------------------------------------------

// The hash table is static and shared by all threads of the user application.
// It is only visible inside this file.  It starts out empty (NULL).  Its size
// is either zero (at the beginning), or a power of two (of size 1024 or more).

#define GB_JITIFIER_INITIAL_SIZE 1024

static GB_jit_entry *GB_jit_table = NULL ;
static int64_t  GB_jit_table_size = 0 ;  // always a power of 2
static uint64_t GB_jit_table_bits = 0 ;  // hash mask (0xFFFF if size is 2^16)
static int64_t  GB_jit_table_populated = 0 ;
static size_t   GB_jit_table_allocated = 0 ;

static char    *GB_jit_cache_path = NULL ;
static size_t   GB_jit_cache_path_allocated = 0 ;

static char    *GB_jit_src_path = NULL ;
static size_t   GB_jit_src_path_allocated = 0 ;

static char    *GB_jit_C_compiler = NULL ;
static size_t   GB_jit_C_compiler_allocated = 0 ;

static char    *GB_jit_C_flags = NULL ;
static size_t   GB_jit_C_flags_allocated = 0 ;

static char    *GB_jit_C_link_flags = NULL ;
static size_t   GB_jit_C_link_flags_allocated = 0 ;

static char    *GB_jit_C_libraries = NULL ;
static size_t   GB_jit_C_libraries_allocated = 0 ;

static char    *GB_jit_C_preface = NULL ;
static size_t   GB_jit_C_preface_allocated = 0 ;

static char    *GB_jit_library_name = NULL ;
static size_t   GB_jit_library_name_allocated = 0 ;

static char    *GB_jit_kernel_name = NULL ;
static size_t   GB_jit_kernel_name_allocated = 0 ;

static char    *GB_jit_command = NULL ;
static size_t   GB_jit_command_allocated = 0 ;

static GxB_JIT_Control GB_jit_control =
    #ifndef NJIT
    GxB_JIT_ON ;        // JIT enabled
    #else
    GxB_JIT_RUN ;       // JIT disabled at compile time; only PreJIT available.
                        // No JIT kernels can be loaded or compiled.
    #endif

//------------------------------------------------------------------------------
// GB_jitifyer_finalize: free the JIT table and all the strings
//------------------------------------------------------------------------------

#define OK(ok)                          \
    if (!(ok))                          \
    {                                   \
        GB_jitifyer_finalize (false) ;  \
        return (GrB_OUT_OF_MEMORY) ;    \
    }

#define GB_FREE_STUFF(X)                                \
{                                                       \
    GB_Global_persistent_free ((void **) &X) ;          \
    X ## _allocated = 0 ;                               \
}

#define GB_MALLOC_STUFF(X,len)                          \
{                                                       \
    X = GB_Global_persistent_malloc ((len) + 2) ;       \
    OK (X != NULL) ;                                    \
    X ## _allocated = (len) + 2 ;                       \
}

#define GB_COPY_STUFF(X,src)                            \
{                                                       \
    size_t len = strlen (src) ;                         \
    GB_MALLOC_STUFF (X, len) ;                          \
    strncpy (X, src, X ## _allocated) ;                 \
}

void GB_jitifyer_finalize (bool freeall)
{ 
    GB_jitifyer_table_free (freeall) ;
    GB_FREE_STUFF (GB_jit_cache_path) ;
    GB_FREE_STUFF (GB_jit_src_path) ;
    GB_FREE_STUFF (GB_jit_C_compiler) ;
    GB_FREE_STUFF (GB_jit_C_flags) ;
    GB_FREE_STUFF (GB_jit_C_link_flags) ;
    GB_FREE_STUFF (GB_jit_C_libraries) ;
    GB_FREE_STUFF (GB_jit_C_preface) ;
    GB_FREE_STUFF (GB_jit_library_name) ;
    GB_FREE_STUFF (GB_jit_kernel_name) ;
    GB_FREE_STUFF (GB_jit_command) ;

    ASSERT (GB_jit_cache_path == NULL) ;
    ASSERT (GB_jit_src_path == NULL) ;
    ASSERT (GB_jit_C_compiler == NULL) ;
    ASSERT (GB_jit_C_flags == NULL) ;
    ASSERT (GB_jit_C_link_flags == NULL) ;
    ASSERT (GB_jit_C_libraries == NULL) ;
    ASSERT (GB_jit_C_preface == NULL) ;
    ASSERT (GB_jit_library_name == NULL) ;
    ASSERT (GB_jit_kernel_name == NULL) ;
    ASSERT (GB_jit_command == NULL) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_init: initialize the CPU and CUDA JIT folders, flags, etc
//------------------------------------------------------------------------------

// Returns GrB_SUCCESS, GrB_OUT_OF_MEMORY, or GrB_NO_VALUE if the cache path
// cannot be found.

GrB_Info GB_jitifyer_init (void)
{

    //--------------------------------------------------------------------------
    // enable the JIT
    //--------------------------------------------------------------------------

    GB_jit_control =
        #ifndef NJIT
        GxB_JIT_ON ;    // JIT enabled
        #else
        GxB_JIT_RUN ;   // JIT disabled at compile time; only PreJIT available.
                        // No JIT kernels can be loaded or compiled.
        #endif

    GB_jitifyer_finalize (true) ;

    //--------------------------------------------------------------------------
    // find the GB_jit_cache_path
    //--------------------------------------------------------------------------

    char *cache_path = getenv ("GRAPHBLAS_CACHE_PATH") ;
    if (cache_path != NULL)
    { 
        // use the environment variable GRAPHBLAS_CACHE_PATH as-is
        GB_COPY_STUFF (GB_jit_cache_path, cache_path) ;
    }
    else
    { 
        // Linux, Mac, Unix: look for HOME
        char *home = getenv ("HOME") ;
        char *dot = "." ;
        if (home == NULL)
        { 
            // Windows: look for LOCALAPPDATA
            home = getenv ("LOCALAPPDATA") ;
            dot = "" ;
        }
        if (home != NULL)
        { 
            // found home; create the cache path
            size_t len = strlen (home) + 60 ;
            GB_MALLOC_STUFF (GB_jit_cache_path, len) ;
            snprintf (GB_jit_cache_path,
                GB_jit_cache_path_allocated,
                "%s/%sSuiteSparse/GraphBLAS/%d.%d.%d", home, dot,
                GxB_IMPLEMENTATION_MAJOR,
                GxB_IMPLEMENTATION_MINOR,
                GxB_IMPLEMENTATION_SUB) ;
        }
    }

    //--------------------------------------------------------------------------
    // establish the cache path and src path, and make sure they exist
    //--------------------------------------------------------------------------

    OK (GB_jitifyer_establish_paths ( ) == GrB_SUCCESS) ;

    //--------------------------------------------------------------------------
    // initialize the remaining strings
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_compiler,   GB_C_COMPILER) ;
    GB_COPY_STUFF (GB_jit_C_flags,      GB_C_FLAGS) ;
    GB_COPY_STUFF (GB_jit_C_link_flags, GB_C_LINK_FLAGS) ;
    GB_COPY_STUFF (GB_jit_C_libraries,  GB_C_LIBRARIES) ;
    GB_COPY_STUFF (GB_jit_C_preface,    "") ;

    //--------------------------------------------------------------------------
    // remove "-arch arm64" if compiling JIT kernels for MATLAB
    //--------------------------------------------------------------------------

    // When the x86-based version of gcc-12 is configured to compile the MATLAB
    // GraphBLAS library on an Apple-Silicon-based Mac, cmake gives it flag
    // "-arch arm64".  MATLAB does not support that architecture directly,
    // using Rosetta 2 instead.  gcc-12 also does not support "-arch arm64", so
    // it ignores it (which is the right thing to do), but it generates a
    // warning.  This warning message appears every time a JIT kernel is
    // compiled.  As a result, "-arch arm64" is removed from the initial C
    // flags, if compiling for MATLAB.

    #ifdef GBMATLAB
    {
        #define ARCH_ARM64 "-arch arm64"
        char *dst = strstr (GB_jit_C_flags, ARCH_ARM64) ;
        if (dst != NULL)
        {
            // found it; now remove it from the C flags
            char *src = dst + strlen (ARCH_ARM64) ;
            while (*dst++ = *src++) ;
        }
    }
    #endif

    //--------------------------------------------------------------------------
    // allocate permanent workspace
    //--------------------------------------------------------------------------

    OK (GB_jitifyer_alloc_space ( ) == GrB_SUCCESS) ;

    //--------------------------------------------------------------------------
    // hash all PreJIT kernels
    //--------------------------------------------------------------------------

    void **Kernels = NULL ;
    void **Queries = NULL ;
    char **Names = NULL ;
    int32_t nkernels = 0 ;
    GB_prejit (&nkernels, &Kernels, &Queries, &Names) ;

    for (int k = 0 ; k < nkernels ; k++)
    {

        //----------------------------------------------------------------------
        // get the name and function pointer of the PreJIT kernel
        //----------------------------------------------------------------------

        void *dl_function = Kernels [k] ;
        GB_jit_query_func dl_query = (GB_jit_query_func) Queries [k] ;
        if (dl_function == NULL || dl_query == NULL || Names [k] == NULL)
        { 
            // ignore this kernel
            continue ;
        }
        char kernel_name [GB_KLEN+1] ;
        strncpy (kernel_name, Names [k], GB_KLEN) ;
        kernel_name [GB_KLEN] = '\0' ;

        //----------------------------------------------------------------------
        // parse the kernel name
        //----------------------------------------------------------------------

        char *name_space = NULL ;
        char *kname = NULL ;
        uint64_t scode = 0 ;
        char *suffix = NULL ;
        GrB_Info info = GB_demacrofy_name (kernel_name, &name_space, &kname,
            &scode, &suffix) ;

        if (info != GrB_SUCCESS || !GB_STRING_MATCH (name_space, "GB_jit"))
        { 
            // kernel_name is invalid; ignore this kernel
            continue ;
        }

        //----------------------------------------------------------------------
        // find the kcode of the kname
        //----------------------------------------------------------------------

        GB_jit_encoding encoding_struct ;
        GB_jit_encoding *encoding = &encoding_struct ;
        memset (encoding, 0, sizeof (GB_jit_encoding)) ;

        #define IS(kernel) GB_STRING_MATCH (kname, kernel)

        GB_jit_kcode c = 0 ;
        if      (IS ("add"          )) c = GB_JIT_KERNEL_ADD ;
        else if (IS ("apply_bind1st")) c = GB_JIT_KERNEL_APPLYBIND1 ;
        else if (IS ("apply_bind2nd")) c = GB_JIT_KERNEL_APPLYBIND2 ;
        else if (IS ("apply_unop"   )) c = GB_JIT_KERNEL_APPLYUNOP ;
        else if (IS ("AxB_dot2"     )) c = GB_JIT_KERNEL_AXB_DOT2 ;
        else if (IS ("AxB_dot2n"    )) c = GB_JIT_KERNEL_AXB_DOT2N ;
        else if (IS ("AxB_dot3"     )) c = GB_JIT_KERNEL_AXB_DOT3 ;
        else if (IS ("AxB_dot4"     )) c = GB_JIT_KERNEL_AXB_DOT4 ;
        else if (IS ("AxB_saxbit"   )) c = GB_JIT_KERNEL_AXB_SAXBIT ;
        else if (IS ("AxB_saxpy3"   )) c = GB_JIT_KERNEL_AXB_SAXPY3 ;
        else if (IS ("AxB_saxpy4"   )) c = GB_JIT_KERNEL_AXB_SAXPY4 ;
        else if (IS ("AxB_saxpy5"   )) c = GB_JIT_KERNEL_AXB_SAXPY5 ;
        else if (IS ("build"        )) c = GB_JIT_KERNEL_BUILD ;
        else if (IS ("colscale"     )) c = GB_JIT_KERNEL_COLSCALE ;
        else if (IS ("concat_bitmap")) c = GB_JIT_KERNEL_CONCAT_BITMAP ;
        else if (IS ("concat_full"  )) c = GB_JIT_KERNEL_CONCAT_FULL ;
        else if (IS ("concat_sparse")) c = GB_JIT_KERNEL_CONCAT_SPARSE ;
        else if (IS ("convert_s2b"  )) c = GB_JIT_KERNEL_CONVERTS2B ;
        else if (IS ("emult_02"     )) c = GB_JIT_KERNEL_EMULT2 ;
        else if (IS ("emult_03"     )) c = GB_JIT_KERNEL_EMULT3 ;
        else if (IS ("emult_04"     )) c = GB_JIT_KERNEL_EMULT4 ;
        else if (IS ("emult_08"     )) c = GB_JIT_KERNEL_EMULT8 ;
        else if (IS ("emult_bitmap" )) c = GB_JIT_KERNEL_EMULT_BITMAP ;
        else if (IS ("ewise_fulla"  )) c = GB_JIT_KERNEL_EWISEFA ;
        else if (IS ("ewise_fulln"  )) c = GB_JIT_KERNEL_EWISEFN ;
        else if (IS ("reduce"       )) c = GB_JIT_KERNEL_REDUCE ;
        else if (IS ("rowscale"     )) c = GB_JIT_KERNEL_ROWSCALE ;
        else if (IS ("select_bitmap")) c = GB_JIT_KERNEL_SELECT_BITMAP ;
        else if (IS ("select_phase1")) c = GB_JIT_KERNEL_SELECT1 ;
        else if (IS ("select_phase2")) c = GB_JIT_KERNEL_SELECT2 ;
        else if (IS ("split_bitmap" )) c = GB_JIT_KERNEL_SPLIT_BITMAP ;
        else if (IS ("split_full"   )) c = GB_JIT_KERNEL_SPLIT_FULL ;
        else if (IS ("split_sparse" )) c = GB_JIT_KERNEL_SPLIT_SPARSE ;
        else if (IS ("subassign_05d")) c = GB_JIT_KERNEL_SUBASSIGN_05d ;
        else if (IS ("subassign_06d")) c = GB_JIT_KERNEL_SUBASSIGN_06d ;
        else if (IS ("subassign_22" )) c = GB_JIT_KERNEL_SUBASSIGN_22 ;
        else if (IS ("subassign_23" )) c = GB_JIT_KERNEL_SUBASSIGN_23 ;
        else if (IS ("subassign_25" )) c = GB_JIT_KERNEL_SUBASSIGN_25 ;
        else if (IS ("trans_bind1st")) c = GB_JIT_KERNEL_TRANSBIND1 ;
        else if (IS ("trans_bind2nd")) c = GB_JIT_KERNEL_TRANSBIND2 ;
        else if (IS ("trans_unop"   )) c = GB_JIT_KERNEL_TRANSUNOP ;
        else if (IS ("union"        )) c = GB_JIT_KERNEL_UNION ;
        else if (IS ("user_op"      )) c = GB_JIT_KERNEL_USEROP ;
        else if (IS ("user_type"    )) c = GB_JIT_KERNEL_USERTYPE ;
        else
        { 
            // kernel_name is invalid; ignore this kernel
            continue ;
        }

        #undef IS
        encoding->kcode = c ;
        encoding->code = scode ;
        encoding->suffix_len = (suffix == NULL) ? 0 : strlen (suffix) ;

        //----------------------------------------------------------------------
        // get the hash of this PreJIT kernel
        //----------------------------------------------------------------------

        // Query the kernel for its hash and version number.  The hash is
        // needed now so the PreJIT kernel can be added to the hash table.

        // The type/op definitions and monoid id/term values for user-defined
        // types/ops/ monoids are ignored, because the user-defined objects
        // have not yet been created during this use of GraphBLAS (this method
        // is called by GrB_init).  These definitions are checked the first
        // time the kernel is run.

        uint64_t hash = 0 ;
        char *ignored [5] ;
        int version [3] ;
        (void) dl_query (&hash, version, ignored, NULL, NULL, 0, 0) ;

        if (hash == 0 || hash == UINT64_MAX ||
            (version [0] != GxB_IMPLEMENTATION_MAJOR) ||
            (version [1] != GxB_IMPLEMENTATION_MINOR) ||
            (version [2] != GxB_IMPLEMENTATION_SUB))
        { 
            // the kernel is stale; ignore it
            continue ;
        }

        //----------------------------------------------------------------------
        // make sure this kernel is not a duplicate
        //----------------------------------------------------------------------

        int64_t k1 = -1, kk = -1 ;
        if (GB_jitifyer_lookup (hash, encoding, suffix, &k1, &kk) != NULL)
        { 
            // the kernel is a duplicate; ignore it
            continue ;
        }

        //----------------------------------------------------------------------
        // insert the PreJIT kernel in the hash table
        //----------------------------------------------------------------------

        if (!GB_jitifyer_insert (hash, encoding, suffix, NULL, dl_function, k))
        { 
            // out of memory
            GB_jit_control = GxB_JIT_PAUSE ;
            return (GrB_OUT_OF_MEMORY) ;
        }
    }

    //--------------------------------------------------------------------------
    // uncompress all the source files into the user source folder
    //--------------------------------------------------------------------------

    return (GB_jitifyer_extract_JITpackage ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_establish_paths: make sure cache and src paths exist
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_establish_paths (void)
{

    //--------------------------------------------------------------------------
    // find the GB_jit_src_path
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_src_path) ;
    if (GB_jit_cache_path != NULL)
    { 
        size_t len = GB_jit_cache_path_allocated + 12 ;
        GB_MALLOC_STUFF (GB_jit_src_path, len) ;
        snprintf (GB_jit_src_path, GB_jit_src_path_allocated, "%s/src",
            GB_jit_cache_path) ;
    }

    //--------------------------------------------------------------------------
    // make sure the cache and source paths exist
    //--------------------------------------------------------------------------

    GrB_Info info1 = GB_jitifyer_mkdir (GB_jit_cache_path) ;
    GrB_Info info2 = GB_jitifyer_mkdir (GB_jit_src_path) ;
    if (info1 != GrB_SUCCESS || info2 != GrB_SUCCESS)
    { 
        // JIT is disabled, or cannot determine the JIT cache and/or source
        // path.  Disable loading and compiling, but continue with the rest of
        // the initializations.  The PreJIT could still be used.
        GBURBLE ("(jit: unable to access cache path, jit disabled) ") ;
        GB_jit_control = GxB_JIT_RUN ;
        GB_FREE_STUFF (GB_jit_cache_path) ;
        GB_FREE_STUFF (GB_jit_src_path) ;
        GB_COPY_STUFF (GB_jit_cache_path, "") ;
        GB_COPY_STUFF (GB_jit_src_path, "") ;
    }

    return (GrB_SUCCESS) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_mkdir: create a directory
//------------------------------------------------------------------------------

// Create a directory, including all parent directories if they do not exist.
// Returns success if the directory already exists or if it was successfully
// created.  Returns GrB_NO_VALUE or GrB_NULL_POINTER on error.  Returns
// GrB_NO_VALUE if the JIT is disabled.

GrB_Info GB_jitifyer_mkdir (char *path)
{
    int result = 0 ;
    int err = -1 ;
    if (path == NULL) return (GrB_NULL_POINTER) ;

    #ifndef NJIT

    // create all the leading directories
    bool first = true ;
    for (char *p = path ; *p ; p++)
    {
        // look for a file separator
        if (*p == '/' || *p == '\\')
        {
            // found a file separator
            if (!first)
            { 
                // terminate the path at this file separator
                char save = *p ;
                *p = '\0' ;
                // construct the directory at this path
                // FIXME: need _mkdir on Windows
                result = mkdir (path, S_IRWXU) ;
                err = (result == -1) ? errno : 0 ;
                // restore the path
                *p = save ;
            }
            first = false ;
        }
    }

    // create the final directory
    // FIXME: need _mkdir on Windows
    result = mkdir (path, S_IRWXU) ;
    err = (result == -1) ? errno : 0 ;
    return ((err == 0 || err == EEXIST) ? GrB_SUCCESS : GrB_NO_VALUE) ;
    #else
    // JIT is disabled at compile time; no need to make any directories
    return (GrB_NO_VALUE) ;
    #endif
}

//------------------------------------------------------------------------------
// GB_jitifyer_extract_JITpackage: extract the GraphBLAS source
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_extract_JITpackage (void)
{

    bool ok = true ;
    #ifndef NJIT

    //--------------------------------------------------------------------------
    // allocate workspace
    //--------------------------------------------------------------------------

    // allocate space for the full path to the output file
    size_t fsize = 0 ;
    char *filename = GB_MALLOC_WORK (GB_jit_src_path_allocated + 300, char,
        &fsize) ;
    if (filename == NULL)
    { 
        // out of memory; disable the JIT
        GB_jit_control = GxB_JIT_RUN ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    // check the GraphBLAS.h file to see if it's OK
    snprintf (filename, fsize, "%s/GraphBLAS.h", GB_jit_src_path) ;
    FILE *fp = fopen (filename, "r") ;
    if (fp != NULL)
    {
        // found the file; read the 1st line for the version number
        int v1 = -1, v2 = -1, v3 = -1 ;
        int r = fscanf (fp, "// SuiteSparse:GraphBLAS %d.%d.%d", &v1, &v2, &v3);
        if (r == 3 &&
            v1 == GxB_IMPLEMENTATION_MAJOR &&
            v2 == GxB_IMPLEMENTATION_MINOR &&
            v3 == GxB_IMPLEMENTATION_SUB)
        { 
            // the header looks fine; assume the rest is OK
            fclose (fp) ;
            GB_FREE_WORK (&filename, fsize) ;
            return (GrB_SUCCESS) ;
        }
    }

    // find the largest uncompressed filesize
    size_t max_uncompressed = 0 ;
    for (int k = 0 ; k < GB_JITpackage_nfiles ; k++)
    { 
        size_t uncompressed_size = GB_JITpackage_index [k].uncompressed_size ;
        max_uncompressed = GB_IMAX (max_uncompressed, uncompressed_size) ;
    }

    // allocate workspace for the largest uncompressed file
    size_t dst_size = 0 ;
    uint8_t *dst = GB_MALLOC_WORK (max_uncompressed+2, uint8_t, &dst_size) ;

    if (dst == NULL || filename == NULL)
    { 
        // out of memory; disable the JIT
        GB_jit_control = GxB_JIT_RUN ;
        GB_FREE_WORK (&dst, dst_size) ;
        GB_FREE_WORK (&filename, fsize) ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    //--------------------------------------------------------------------------
    // uncompress each file
    //--------------------------------------------------------------------------

    for (int k = 0 ; k < GB_JITpackage_nfiles ; k++)
    { 
        // uncompress the blob
        uint8_t *src = GB_JITpackage_index [k].blob ;
        size_t src_size = GB_JITpackage_index [k].compressed_size ;
        size_t u = ZSTD_decompress (dst, dst_size, src, src_size) ;
        if (u != GB_JITpackage_index [k].uncompressed_size)
        { 
            // blob is invalid
            ok = false ;
            break ;
        }
        // construct the filename
        snprintf (filename, fsize, "%s/%s", GB_jit_src_path,
            GB_JITpackage_index [k].filename) ;
        // open the file
        fp = fopen (filename, "w") ;
        if (fp == NULL)
        { 
            // file cannot be created
            ok = false ;
            break ;
        }
        // write the uncompressed blob to the file
        size_t nwritten = fwrite (dst, sizeof (uint8_t), u, fp) ;
        fclose (fp) ;
        if (nwritten != u)
        { 
            // file is invalid
            ok = false ;
            break ;
        }
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    GB_FREE_WORK (&dst, dst_size) ;
    GB_FREE_WORK (&filename, fsize) ;
    #endif

    #pragma omp flush
    return (ok ? GrB_SUCCESS : GrB_NO_VALUE) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_control: get the JIT control
//------------------------------------------------------------------------------

GxB_JIT_Control GB_jitifyer_get_control (void)
{
    GxB_JIT_Control control ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        control = GB_jit_control ;
    }
    return (control) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_control: set the JIT control
//------------------------------------------------------------------------------

void GB_jitifyer_set_control (int control)
{
    #pragma omp critical (GB_jitifyer_worker)
    {
        control = GB_IMAX (control, GxB_JIT_OFF) ;
        #ifndef NJIT
        // The full JIT is available.
        control = GB_IMIN (control, GxB_JIT_ON) ;
        #else
        // The JIT is restricted; only OFF, PAUSE, and RUN settings can be
        // used.  No JIT kernels can be loaded or compiled.
        control = GB_IMIN (control, GxB_JIT_RUN) ;
        #endif
        GB_jit_control = (GxB_JIT_Control) control ;
        if (GB_jit_control == GxB_JIT_OFF)
        { 
            // free all loaded JIT kernels but do not free the JIT hash table,
            // and do not free the PreJIT kernels
            GB_jitifyer_table_free (false) ;
        }
    }
}

//------------------------------------------------------------------------------
// GB_jitifyer_alloc_space: allocate workspaces for the JIT
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_alloc_space (void)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (GB_jit_C_flags == NULL ||
        GB_jit_C_link_flags == NULL ||
        GB_jit_C_libraries == NULL ||
        GB_jit_cache_path == NULL ||
        GB_jit_src_path == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // allocate GB_jit_kernel_name if needed
    //--------------------------------------------------------------------------

    if (GB_jit_kernel_name == NULL)
    { 
        size_t len = GB_jit_cache_path_allocated + 300 + 2 * GxB_MAX_NAME_LEN ;
        GB_MALLOC_STUFF (GB_jit_kernel_name, len) ;
    }

    //--------------------------------------------------------------------------
    // allocate GB_jit_library_name if needed
    //--------------------------------------------------------------------------

    if (GB_jit_library_name == NULL)
    { 
        size_t len = GB_jit_cache_path_allocated + 300 + 2 * GxB_MAX_NAME_LEN ;
        GB_MALLOC_STUFF (GB_jit_library_name, len) ;
    }

    //--------------------------------------------------------------------------
    // allocate GB_jit_command if needed
    //--------------------------------------------------------------------------

    if (GB_jit_command == NULL)
    { 
        size_t len =
            2 * GB_jit_C_compiler_allocated +
            2 * GB_jit_C_flags_allocated +
            GB_jit_C_link_flags_allocated +
            GB_jit_src_path_allocated +
            strlen (GB_OMP_INC) +
            4 * GB_jit_cache_path_allocated + 5 * GB_KLEN +
            GB_jit_C_libraries_allocated +
            300 ;
        GB_MALLOC_STUFF (GB_jit_command, len) ;
    }

    return (GrB_SUCCESS) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_cache_path: return the current cache path
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_cache_path (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_cache_path ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_cache_path: set a new cache path
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_cache_path (const char *new_cache_path)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_cache_path == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the cache path in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_cache_path_worker (new_cache_path) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_cache_path_worker: set cache path in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_cache_path_worker (const char *new_cache_path)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the cache path
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_cache_path) ;
    GB_FREE_STUFF (GB_jit_kernel_name) ;
    GB_FREE_STUFF (GB_jit_library_name) ;
    GB_FREE_STUFF (GB_jit_command) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_cache_path
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_cache_path, new_cache_path) ;

    //--------------------------------------------------------------------------
    // set the src path and make sure cache and src paths are accessible
    //--------------------------------------------------------------------------

    OK (GB_jitifyer_establish_paths ( ) == GrB_SUCCESS) ;

    //--------------------------------------------------------------------------
    // allocate and define strings that depend on GB_jit_cache_path
    //--------------------------------------------------------------------------

    OK (GB_jitifyer_alloc_space ( ) == GrB_SUCCESS) ;

    //--------------------------------------------------------------------------
    // uncompress all the source files into the user source folder
    //--------------------------------------------------------------------------

    return (GB_jitifyer_extract_JITpackage ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_C_compiler: return the current C compiler
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_C_compiler (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_C_compiler ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_compiler: set a new C compiler
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_compiler (const char *new_C_compiler)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_C_compiler == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the C compiler in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_C_compiler_worker (new_C_compiler) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_compiler_worker: set C compiler in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_compiler_worker (const char *new_C_compiler)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the C compiler
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_C_compiler) ;
    GB_FREE_STUFF (GB_jit_command) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_C_compiler
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_compiler, new_C_compiler) ;

    //--------------------------------------------------------------------------
    // allocate and define strings that depend on GB_jit_C_compiler
    //--------------------------------------------------------------------------

    return (GB_jitifyer_alloc_space ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_C_flags: return the current C flags
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_C_flags (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_C_flags ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_flags: set new C flags
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_flags (const char *new_C_flags)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_C_flags == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the C flags in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_C_flags_worker (new_C_flags) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_flags_worker: set C flags in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_flags_worker (const char *new_C_flags)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the C flags
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_C_flags) ;
    GB_FREE_STUFF (GB_jit_command) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_C_flags
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_flags, new_C_flags) ;

    //--------------------------------------------------------------------------
    // allocate and define strings that depend on GB_jit_C_flags
    //--------------------------------------------------------------------------

    return (GB_jitifyer_alloc_space ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_C_link_flags: return the current C link flags
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_C_link_flags (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_C_link_flags ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_link_flags: set new C link flags
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_link_flags (const char *new_C_link_flags)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_C_link_flags == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the C link flags in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_C_link_flags_worker (new_C_link_flags) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_link_flags_worker: set C link flags in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_link_flags_worker (const char *new_C_link_flags)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the C flags
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_C_link_flags) ;
    GB_FREE_STUFF (GB_jit_command) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_C_link_flags
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_link_flags, new_C_link_flags) ;

    //--------------------------------------------------------------------------
    // allocate and define strings that depend on GB_jit_C_link_flags
    //--------------------------------------------------------------------------

    return (GB_jitifyer_alloc_space ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_C_libraries: return the current C libraries
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_C_libraries (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_C_libraries ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_libraries: set new C libraries
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_libraries (const char *new_C_libraries)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_C_libraries == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the C libraries in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_C_libraries_worker (new_C_libraries) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_libraries_worker: set C libraries in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_libraries_worker (const char *new_C_libraries)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the C libraries
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_C_libraries) ;
    GB_FREE_STUFF (GB_jit_command) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_C_libraries
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_libraries, new_C_libraries) ;

    //--------------------------------------------------------------------------
    // allocate and define strings that depend on GB_jit_C_libraries
    //--------------------------------------------------------------------------

    return (GB_jitifyer_alloc_space ( )) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_get_C_preface: return the current C libraries
//------------------------------------------------------------------------------

const char *GB_jitifyer_get_C_preface (void)
{
    const char *s ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        s = GB_jit_C_preface ;
    }
    return (s) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_preface: set new C libraries
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_preface (const char *new_C_preface)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    if (new_C_preface == NULL)
    { 
        return (GrB_NULL_POINTER) ;
    }

    //--------------------------------------------------------------------------
    // set the C preface in a critical section
    //--------------------------------------------------------------------------

    GrB_Info info ;
    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_set_C_preface_worker (new_C_preface) ;
    }
    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_set_C_preface_worker: set C libraries in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_set_C_preface_worker (const char *new_C_preface)
{ 

    //--------------------------------------------------------------------------
    // free the old strings that depend on the C preface
    //--------------------------------------------------------------------------

    GB_FREE_STUFF (GB_jit_C_preface) ;

    //--------------------------------------------------------------------------
    // allocate the new GB_jit_C_preface
    //--------------------------------------------------------------------------

    GB_COPY_STUFF (GB_jit_C_preface, new_C_preface) ;
    return (GrB_SUCCESS) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_query: check if the type/op/monoid definitions match
//------------------------------------------------------------------------------

bool GB_jitifyer_query
(
    GB_jit_query_func dl_query,
    uint64_t hash,              // hash code for the kernel
    // operator and type definitions
    GrB_Semiring semiring,
    GrB_Monoid monoid,
    GB_Operator op,
    GrB_Type type1,
    GrB_Type type2,
    GrB_Type type3
)
{

    //--------------------------------------------------------------------------
    // get the terms to query
    //--------------------------------------------------------------------------

    int version [3] ;
    char *library_defn [5] ;
    size_t zsize = 0 ;
    size_t tsize = 0 ;
    void *id = NULL ;
    void *term = NULL ;

    GB_Operator op1 = NULL, op2 = NULL ;
    if (semiring != NULL)
    { 
        monoid = semiring->add ;
        op1 = (GB_Operator) monoid->op ;
        op2 = (GB_Operator) semiring->multiply ;
    }
    else if (monoid != NULL)
    { 
        op1 = (GB_Operator) monoid->op ;
    }
    else
    { 
        // op may be NULL, if this is a user_type kernel
        op1 = op ;
    }

    if (monoid != NULL && monoid->hash != 0)
    { 
        // compare the user-defined identity and terminal values
        zsize = monoid->op->ztype->size ;
        tsize = (monoid->terminal == NULL) ? 0 : zsize ;
        id = monoid->identity ;
        term = monoid->terminal ;
    }

    //--------------------------------------------------------------------------
    // query the JIT kernel for its definitions
    //--------------------------------------------------------------------------

    uint64_t hash2 = 0 ;
    bool ok = dl_query (&hash2, version, library_defn, id, term, zsize, tsize) ;
    ok = ok && (version [0] == GxB_IMPLEMENTATION_MAJOR) &&
               (version [1] == GxB_IMPLEMENTATION_MINOR) &&
               (version [2] == GxB_IMPLEMENTATION_SUB) &&
               (hash == hash2) ;

    //--------------------------------------------------------------------------
    // compare current definitions with the ones in the JIT kernel
    //--------------------------------------------------------------------------

    char *defn [5] ;
    defn [0] = (op1 == NULL) ? NULL : op1->defn ;
    defn [1] = (op2 == NULL) ? NULL : op2->defn ;
    defn [2] = (type1 == NULL) ? NULL : type1->defn ;
    defn [3] = (type2 == NULL) ? NULL : type2->defn ;
    defn [4] = (type3 == NULL) ? NULL : type3->defn ;

    for (int k = 0 ; k < 5 ; k++)
    {
        if ((defn [k] != NULL) != (library_defn [k] != NULL))
        { 
            // one is not NULL but the other is NULL
            ok = false ;
        }
        else if (defn [k] != NULL)
        { 
            // both definitions are present
            // ensure the definition hasn't changed
            ok = ok && (strcmp (defn [k], library_defn [k]) == 0) ;
        }
        else
        { 
            // both definitions are NULL, so they match
        }
    }
    return (ok) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_load: load a JIT kernel, compiling it if needed
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_load
(
    // output:
    void **dl_function,         // pointer to JIT kernel
    // input:
    GB_jit_family family,       // kernel family
    const char *kname,          // kname for the kernel_name
    uint64_t hash,              // hash code for the kernel
    GB_jit_encoding *encoding,  // encoding of the problem
    const char *suffix,         // suffix for the kernel_name (NULL if none)
    // operator and type definitions
    GrB_Semiring semiring,
    GrB_Monoid monoid,
    GB_Operator op,
    GrB_Type type1,
    GrB_Type type2,
    GrB_Type type3
)
{

    //--------------------------------------------------------------------------
    // check for quick return
    //--------------------------------------------------------------------------

    GrB_Info info ;
    if (hash == UINT64_MAX)
    { 
        // The kernel may not be compiled; it does not have a valid definition.
        GBURBLE ("(jit undefined) ") ;
        return (GrB_NO_VALUE) ;
    }

    if (GB_jit_control <= GxB_JIT_PAUSE)
    { 
        // The JIT control has disabled all JIT kernels.  Punt to generic.
        GBURBLE ("(jit paused) ") ;
        return (GrB_NO_VALUE) ;
    }

    //--------------------------------------------------------------------------
    // handle the GxB_JIT_RUN case: critical section not required
    //--------------------------------------------------------------------------

    if (GB_jit_control == GxB_JIT_RUN &
        family != GB_jit_user_op_family && family != GB_jit_user_type_family)
    {

        //----------------------------------------------------------------------
        // look up the kernel in the hash table
        //----------------------------------------------------------------------

        int64_t k1 = -1, kk = -1 ;
        (*dl_function) = GB_jitifyer_lookup (hash, encoding, suffix, &k1, &kk) ;
        if (k1 >= 0)
        { 
            // an unchecked PreJIT kernel; check it inside critical section
        }
        else if ((*dl_function) != NULL)
        { 
            // found the kernel in the hash table
            GBURBLE ("(jit run) ") ;
            return (GrB_SUCCESS) ;
        }
        else
        { 
            // No kernels may be loaded or compiled, but existing kernels
            // already loaded may be run (handled above if dl_function was
            // found).  This kernel was not loaded, so punt to generic.
            GBURBLE ("(jit not loaded) ") ;
            return (GrB_NO_VALUE) ;
        }
    }

    //--------------------------------------------------------------------------
    // do the rest inside a critical section
    //--------------------------------------------------------------------------

    #pragma omp critical (GB_jitifyer_worker)
    { 
        info = GB_jitifyer_worker (dl_function, family, kname, hash,
            encoding, suffix, semiring, monoid, op, type1, type2, type3) ;
    }

    return (info) ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_worker: do the work for GB_jitifyer_load in a critical section
//------------------------------------------------------------------------------

GrB_Info GB_jitifyer_worker
(
    // output:
    void **dl_function,         // pointer to JIT kernel
    // input:
    GB_jit_family family,       // kernel family
    const char *kname,          // kname for the kernel_name
    uint64_t hash,              // hash code for the kernel
    GB_jit_encoding *encoding,  // encoding of the problem
    const char *suffix,         // suffix for the kernel_name (NULL if none)
    // operator and type definitions
    GrB_Semiring semiring,
    GrB_Monoid monoid,
    GB_Operator op,
    GrB_Type type1,
    GrB_Type type2,
    GrB_Type type3
)
{

    //--------------------------------------------------------------------------
    // look up the kernel in the hash table
    //--------------------------------------------------------------------------

    int64_t k1 = -1, kk = -1 ;
    (*dl_function) = GB_jitifyer_lookup (hash, encoding, suffix, &k1, &kk) ;
    if ((*dl_function) != NULL)
    { 
        // found the kernel in the hash table
        GB_jit_entry *e = &(GB_jit_table [kk]) ;
        if (k1 >= 0)
        {
            // unchecked PreJIT kernel; check it now
            void **Kernels = NULL ;
            void **Queries = NULL ;
            char **Names = NULL ;
            int32_t nkernels = 0 ;
            GB_prejit (&nkernels, &Kernels, &Queries, &Names) ;
            GB_jit_query_func dl_query = (GB_jit_query_func) Queries [k1] ;
            bool ok = GB_jitifyer_query (dl_query, hash, semiring, monoid, op,
                type1, type2, type3) ;
            if (ok)
            { 
                // PreJIT kernel is fine; flag it as checked by flipping
                // its prejit_index.
                GBURBLE ("(prejit: ok) ") ;
                e->prejit_index = GB_FLIP (k1) ;
                return (GrB_SUCCESS) ;
            }
            else
            { 
                // remove the PreJIT kernel from the hash table
                GBURBLE ("(prejit: disabled) ") ;
                GB_jitifyer_entry_free (e) ;
            }
        }
        else if (family == GB_jit_user_op_family)
        {
            // user-defined operator; check it now
            GB_user_op_f GB_user_op = (GB_user_op_f) (*dl_function) ;
            void *ignore ;
            char *defn ;
            GB_user_op (&ignore, &defn) ;
            if (strcmp (defn, op->defn) == 0)
            { 
                GBURBLE ("(jit op: ok) ") ;
                return (GrB_SUCCESS) ;
            }
            else
            { 
                // the op has changed; need to re-JIT the kernel
                GBURBLE ("(jit op: changed) ") ;
                GB_jitifyer_entry_free (e) ;
            }
        }
        else if (family == GB_jit_user_type_family)
        {
            // user-defined type; check it now
            GB_user_type_f GB_user_type = (GB_user_type_f) (*dl_function) ;
            size_t ignore ;
            char *defn ;
            GB_user_type (&ignore, &defn) ;
            if (strcmp (defn, type1->defn) == 0)
            { 
                GBURBLE ("(jit type: ok) ") ;
                return (GrB_SUCCESS) ;
            }
            else
            { 
                // the type has changed; need to re-JIT the kernel
                GBURBLE ("(jit type: changed) ") ;
                GB_jitifyer_entry_free (e) ;
            }
        }
        else
        { 
            // JIT kernel, or checked PreJIT kernel
            GBURBLE ("(jit run) ") ;
            return (GrB_SUCCESS) ;
        }
    }

    //--------------------------------------------------------------------------
    // quick return if not in the hash table
    //--------------------------------------------------------------------------

    #ifndef NJIT
    if (GB_jit_control <= GxB_JIT_RUN)
    #endif
    { 
        // No kernels may be loaded or compiled, but existing kernels already
        // loaded may be run (handled above if dl_function was found).  This
        // kernel was not loaded, so punt to generic.
        GBURBLE ("(jit not loaded) ") ;
        return (GrB_NO_VALUE) ;
    }

    //--------------------------------------------------------------------------
    // the kernel needs to be loaded, and perhaps compiled; get its properties
    //--------------------------------------------------------------------------

    #ifndef NJIT

    GB_Operator op1 = NULL ;
    GB_Operator op2 = NULL ;
    int scode_digits = 0 ;

    switch (family)
    {
        case GB_jit_apply_family  : 
            op1 = op ;
            scode_digits = 10 ;
            break ;

        case GB_jit_assign_family : 
            op1 = op ;
            scode_digits = 12 ;
            break ;

        case GB_jit_build_family  : 
            op1 = op ;
            scode_digits = 7 ;
            break ;

        case GB_jit_ewise_family  : 
            op1 = op ;
            scode_digits = 13 ;
            break ;

        case GB_jit_mxm_family    : 
            monoid = semiring->add ;
            op1 = (GB_Operator) semiring->add->op ;
            op2 = (GB_Operator) semiring->multiply ;
            scode_digits = 16 ;
            break ;

        case GB_jit_reduce_family : 
            op1 = (GB_Operator) monoid->op ;
            scode_digits = 7 ;
            break ;

        case GB_jit_select_family : 
            op1 = op ;
            scode_digits = 10 ;
            break ;

        case GB_jit_user_type_family : 
            scode_digits = 1 ;
            break ;

        case GB_jit_user_op_family : 
            scode_digits = 1 ;
            op1 = op ;
            break ;

        default: ;
    }

    //--------------------------------------------------------------------------
    // name the problem
    //--------------------------------------------------------------------------

    char kernel_name [GB_KLEN] ;
    GB_macrofy_name (kernel_name, "GB_jit", kname, scode_digits,
        encoding->code, suffix) ;

    //--------------------------------------------------------------------------
    // try to load the libkernel_name.so from the user's library folder
    //--------------------------------------------------------------------------

    snprintf (GB_jit_library_name, GB_jit_library_name_allocated,
        "%s/lib%s%s", GB_jit_cache_path, kernel_name, GB_LIB_SUFFIX) ;
    // FIXME: dlopen only exists on Linux/Unix/Mac
    void *dl_handle = dlopen (GB_jit_library_name, RTLD_LAZY) ;

    //--------------------------------------------------------------------------
    // check if the kernel was found, but needs to be compiled anyway
    //--------------------------------------------------------------------------

    bool builtin = (encoding->suffix_len == 0) ;
    if (dl_handle != NULL)
    { 
        // library is loaded but make sure the defn are OK
        // FIXME: dlsym only exists on Linux/Unix/Mac
        GB_jit_query_func dl_query = (GB_jit_query_func)
            dlsym (dl_handle, "GB_jit_query") ;

        bool ok = true ;
        if (dl_query == NULL)
        { 
            // library is missing the GB_jit_query method
            ok = false ;
            GB_jit_control = GxB_JIT_RUN ;
            return (GrB_INVALID_VALUE) ;
        }

        if (ok)
        { 
            ok = GB_jitifyer_query (dl_query, hash, semiring, monoid, op,
                type1, type2, type3) ;
        }

        if (!ok)
        { 
            // library is loaded but needs to change, so close it
            // FIXME: dlclose only exists on Linux/Unix/Mac
            GBURBLE ("(jit: loaded but must recompile) ") ;
            dlclose (dl_handle) ;
            dl_handle = NULL ;
            if (GB_jit_control == GxB_JIT_LOAD)
            { 
                // If the JIT control is set to GxB_JIT_LOAD, new kernels
                // cannot be compiled.  This kernel has just been loaded but it
                // has stale definition.  Loading it again will result in the
                // same issue, but will take a lot of time if the kernel is
                // loaded again and again, since no new kernels can be
                // compiled.  Set the JIT control to GxB_JIT_RUN to avoid this
                // performance issue.
                GB_jit_control = GxB_JIT_RUN ;
                return (GrB_INVALID_VALUE) ;
            }
        }
    }

    //--------------------------------------------------------------------------
    // create and compile source file, if needed
    //--------------------------------------------------------------------------

    if (dl_handle == NULL)
    { 

        //----------------------------------------------------------------------
        // quick return if the JIT is not permitted to compile new kernels
        //----------------------------------------------------------------------

        if (GB_jit_control < GxB_JIT_ON)
        { 
            // No new kernels may be compiled, so punt to generic.
            GBURBLE ("(jit not compiled) ") ;
            return (GrB_NO_VALUE) ;
        }

        //----------------------------------------------------------------------
        // create the kernel source file
        //----------------------------------------------------------------------

        GBURBLE ("(jit compile and load) ") ;
        snprintf (GB_jit_kernel_name, GB_jit_kernel_name_allocated,
            "%s/%s.c", GB_jit_cache_path, kernel_name) ;
        FILE *fp = fopen (GB_jit_kernel_name, "w") ;
        if (fp == NULL)
        { 
            // disable the JIT to avoid repeated compilation errors
            GBURBLE ("(jit: cannot create kernel; compilation disabled) ") ;
            GB_jit_control = GxB_JIT_LOAD ;
            return (GrB_INVALID_VALUE) ;
        }

        // create the header and copyright
        fprintf (fp,
            "//--------------------------------------"
            "----------------------------------------\n"
            "// %s.c\n", kernel_name) ;
        GB_macrofy_copyright (fp) ;

        // add the preface, which is an empty string by default
        fprintf (fp, "%s\n", GB_jit_C_preface) ;

        // #include the GB_jit_kernel.h header file
        fprintf (fp, "#include \"GB_jit_kernel.h\"\n\n") ;

        // macrofy the kernel operators, types, and matrix formats
        GB_macrofy_family (fp, family, encoding->code, semiring, monoid,
            op, type1, type2, type3) ;

        // include the kernel, renaming it for the PreJIT
        fprintf (fp, "#ifndef GB_JIT_RUNTIME\n"
                     "#define GB_jit_kernel %s\n"
                     "#define GB_jit_query  %s_query\n"
                     "#endif\n"
                     "#include \"GB_jit_kernel_%s.c\"\n",
                     kernel_name, kernel_name, kname) ;

        // macrofy the query function
        GB_macrofy_query (fp, builtin, monoid, op1, op2, type1, type2, type3,
            hash) ;
        fclose (fp) ;

        //----------------------------------------------------------------------
        // compile the source file to create the lib*.so file
        //----------------------------------------------------------------------

        GB_jitifyer_compile (kernel_name) ;
        // FIXME: dlopen only exists on Linux/Unix/Mac
        dl_handle = dlopen (GB_jit_library_name, RTLD_LAZY) ;
        if (dl_handle == NULL)
        { 
            // unable to open lib*.so file
            GBURBLE ("(jit: compiler error; compilation disabled) ") ;
            // disable the JIT to avoid repeated compilation errors
            GB_jit_control = GxB_JIT_LOAD ;
            return (GrB_INVALID_VALUE) ;
        }
    }
    else
    { 
        GBURBLE ("(jit load) ") ;
    }

    //--------------------------------------------------------------------------
    // get the jit_kernel_function pointer
    //--------------------------------------------------------------------------

    // FIXME: dlsym only exists on Linux/Unix/Mac
    (*dl_function) = dlsym (dl_handle, "GB_jit_kernel") ;
    if ((*dl_function) == NULL)
    { 
        // unable to find GB_jit_kernel: punt to generic
        GBURBLE ("(jit: load error; JIT loading disabled) ") ;
        // FIXME: dlclose only exists on Linux/Unix/Mac
        dlclose (dl_handle) ; 
        dl_handle = NULL ;
        // disable the JIT to avoid repeated loading errors
        GB_jit_control = GxB_JIT_RUN ;
        return (GrB_INVALID_VALUE) ;
    }

    // insert the new kernel into the hash table
    if (!GB_jitifyer_insert (hash, encoding, suffix, dl_handle, (*dl_function),
        -1))
    { 
        // unable to add kernel to hash table: punt to generic
        // FIXME: dlclose only exists on Linux/Unix/Mac
        dlclose (dl_handle) ; 
        dl_handle = NULL ;
        // disable the JIT to avoid repeated errors
        GB_jit_control = GxB_JIT_PAUSE ;
        return (GrB_OUT_OF_MEMORY) ;
    }

    return (GrB_SUCCESS) ;
    #endif
}

//------------------------------------------------------------------------------
// GB_jitifyer_lookup:  find a jit entry in the hash table
//------------------------------------------------------------------------------

void *GB_jitifyer_lookup    // return dl_function pointer, or NULL if not found
(
    // input:
    uint64_t hash,          // hash = GB_jitifyer_hash_encoding (encoding) ;
    GB_jit_encoding *encoding,
    const char *suffix,
    // output
    int64_t *k1,            // location of unchecked kernel in PreJIT table
    int64_t *kk             // location of hash entry in hash table
)
{

    (*k1) = -1 ;

    if (GB_jit_table == NULL)
    { 
        // no table yet so it isn't present
        return (NULL) ;
    }

    uint32_t suffix_len = encoding->suffix_len ;
    bool builtin = (bool) (suffix_len == 0) ;

    // look up the entry in the hash table
    for (int64_t k = hash ; ; k++)
    {
        k = k & GB_jit_table_bits ;
        GB_jit_entry *e = &(GB_jit_table [k]) ;
        if (e->dl_function == NULL)
        { 
            // found an empty entry, so the entry is not in the table
            return (NULL) ;
        }
        else if (e->hash == hash &&
            e->encoding.code == encoding->code &&
            e->encoding.kcode == encoding->kcode &&
            e->encoding.suffix_len == suffix_len &&
            (builtin || (memcmp (e->suffix, suffix, suffix_len) == 0)))
        { 
            // found the right entry: return the corresponding dl_function
            int64_t my_k1 ;
            GB_ATOMIC_READ
            my_k1 = e->prejit_index ;   // >= 0: unchecked JIT kernel
            (*k1) = my_k1 ;
            (*kk) = k ;
            return (e->dl_function) ;
        }
        // otherwise, keep looking
    }
}

//------------------------------------------------------------------------------
// GB_jitifyer_insert:  insert a jit entry in the hash table
//------------------------------------------------------------------------------

bool GB_jitifyer_insert         // return true if successful, false if failure
(
    // input:
    uint64_t hash,              // hash for the problem
    GB_jit_encoding *encoding,  // primary encoding
    const char *suffix,         // suffix for user-defined types/operators
    void *dl_handle,            // library handle from dlopen; NULL for PreJIT
    void *dl_function,          // function handle from dlsym
    int32_t prejit_index        // index into PreJIT table; =>0 if unchecked.
)
{

    size_t siz = 0 ;

    //--------------------------------------------------------------------------
    // ensure the hash table is large enough
    //--------------------------------------------------------------------------

    if (GB_jit_table == NULL)
    { 

        //----------------------------------------------------------------------
        // allocate the initial hash table
        //----------------------------------------------------------------------

        siz = GB_JITIFIER_INITIAL_SIZE * sizeof (struct GB_jit_entry_struct) ;
        GB_jit_table = GB_Global_persistent_malloc (siz) ;
        if (GB_jit_table == NULL)
        { 
            // out of memory
            return (false) ;
        }
        memset (GB_jit_table, 0, siz) ;
        GB_jit_table_size = GB_JITIFIER_INITIAL_SIZE ;
        GB_jit_table_bits = GB_JITIFIER_INITIAL_SIZE - 1 ; 
        GB_jit_table_allocated = siz ;

    }
    else if (4 * GB_jit_table_populated >= GB_jit_table_size)
    {

        //----------------------------------------------------------------------
        // expand the existing hash table by a factor of 4 and rehash
        //----------------------------------------------------------------------

        // create a new table that is four times the size
        int64_t new_size = 4 * GB_jit_table_size ;
        int64_t new_bits = new_size - 1 ;
        siz = new_size * sizeof (struct GB_jit_entry_struct) ;
        GB_jit_entry *new_table = GB_Global_persistent_malloc (siz) ;
        if (GB_jit_table == NULL)
        { 
            // out of memory; leave the existing table as-is
            return (false) ;
        }

        // rehash into the new table
        memset (new_table, 0, siz) ;
        for (int64_t k = 0 ; k < GB_jit_table_size ; k++)
        {
            if (GB_jit_table [k].dl_function != NULL)
            { 
                // rehash the entry to the larger hash table
                uint64_t hash = GB_jit_table [k].hash ;
                new_table [hash & new_bits] = GB_jit_table [k] ;
            }
        }

        // free the old table
        GB_FREE_STUFF (GB_jit_table) ;

        // use the new table
        GB_jit_table = new_table ;
        GB_jit_table_size = new_size ;
        GB_jit_table_bits = new_bits ;
        GB_jit_table_allocated = siz ;
    }

    //--------------------------------------------------------------------------
    // insert the jit entry in the hash table
    //--------------------------------------------------------------------------

    uint32_t suffix_len = encoding->suffix_len ;
    bool builtin = (bool) (suffix_len == 0) ;

    for (int64_t k = hash ; ; k++)
    {
        k = k & GB_jit_table_bits ;
        GB_jit_entry *e = &(GB_jit_table [k]) ;
        if (e->dl_function == NULL)
        { 
            // found an empty slot
            e->suffix = NULL ;
            if (!builtin)
            { 
                // allocate the suffix if the kernel is not builtin
                e->suffix = GB_Global_persistent_malloc (suffix_len+1) ;
                if (e->suffix == NULL)
                { 
                    // out of memory
                    return (false) ;
                }
                strncpy (e->suffix, suffix, suffix_len+1) ;
            }
            e->hash = hash ;
            memcpy (&(e->encoding), encoding, sizeof (GB_jit_encoding)) ;
            e->dl_handle = dl_handle ;              // NULL for PreJIT
            e->dl_function = dl_function ;
            e->prejit_index = prejit_index ;        // -1 for JIT kernels
            GB_jit_table_populated++ ;
            return (true) ;
        }
        // otherwise, keep looking
    }
}

//------------------------------------------------------------------------------
// GB_jitifyer_entry_free: free a single JIT hash table entry
//------------------------------------------------------------------------------

void GB_jitifyer_entry_free (GB_jit_entry *e)
{
    e->dl_function = NULL ;
    GB_Global_persistent_free ((void **) (&(e->suffix))) ;
    // unload the dl library
    if (e->dl_handle != NULL)
    {
        #ifndef NJIT
        // FIXME: dlclose only exists on Linux/Unix/Mac
        dlclose (e->dl_handle) ;
        #endif
        e->dl_handle = NULL ;
    }
    GB_jit_table_populated-- ;
}

//------------------------------------------------------------------------------
// GB_jitifyer_table_free:  free the hash and clear all loaded kernels
//------------------------------------------------------------------------------

// Clears all runtime JIT kernels from the hash table.  PreJIT kernels and JIT
// kernels containing user-defined operators are not freed if freall is true
// (only done by GrB_finalize), but they are flagged as unchecked.  This allows
// the application to call GxB_set to set the JIT control to OFF then ON again,
// to indicate that a user-defined type or operator has been changed, and that
// all JIT kernels must cleared and all PreJIT kernels checked again before
// using them.

// After calling this function, the JIT is still enabled.  GB_jitifyer_insert
// will reallocate the table if it is NULL.

void GB_jitifyer_table_free (bool freeall)
{ 
    if (GB_jit_table != NULL)
    {
        for (int64_t k = 0 ; k < GB_jit_table_size ; k++)
        {
            GB_jit_entry *e = &(GB_jit_table [k]) ;
            if (e->dl_function != NULL)
            {
                // found an entry
                if (e->dl_handle == NULL)
                { 
                    // flag the PreJIT kernel as unchecked
                    e->prejit_index = GB_UNFLIP (e->prejit_index) ;
                }
                // free it if permitted
                if (freeall || (e->dl_handle != NULL &&
                      e->encoding.kcode != GB_JIT_KERNEL_USEROP))
                { 
                    // free the entry
                    GB_jitifyer_entry_free (e) ;
                }
            }
        }
    }

    if (GB_jit_table_populated == 0)
    { 
        // the JIT table is now empty, so free it
        GB_FREE_STUFF (GB_jit_table) ;
        GB_jit_table_size = 0 ;
        GB_jit_table_bits = 0 ;
    }
}

//------------------------------------------------------------------------------
// GB_jitifyer_compile: compile a kernel
//------------------------------------------------------------------------------

#ifndef NJIT

// If the runtime JIT is disabled, no new JIT kernels may be compiled at run
// time.  The PreJIT may still be used.

int GB_jitifyer_compile (char *kernel_name)
{ 

    snprintf (GB_jit_command, GB_jit_command_allocated,

    // compile:
    "%s -DGB_JIT_RUNTIME=1 "        // compiler command
    "%s "                           // C flags
    "-I%s "                         // include source directory
    "%s "                           // openmp include directories
    "-o %s/%s%s "                   // *.o output file
    "-c %s/%s.c ; "                 // *.c input file

    // link:
    "%s "                           // C compiler
    "%s "                           // C flags
    "%s "                           // C link flags
    "-o %s/lib%s%s "                // lib*.so output file
    "%s/%s%s "                      // *.o input file
    "%s "                           // libraries to link with
    ,

    // compile:
    GB_jit_C_compiler,              // C compiler
    GB_jit_C_flags,                 // C flags
    GB_jit_src_path,                // include source directory
    GB_OMP_INC,                     // openmp include
    GB_jit_cache_path, kernel_name,
    GB_OBJ_SUFFIX,                  // *.o output file
    GB_jit_cache_path, kernel_name, // *.c input file

    // link:
    GB_jit_C_compiler,              // C compiler
    GB_jit_C_flags,                 // C flags
    GB_jit_C_link_flags,            // C link flags
    GB_jit_cache_path, kernel_name, GB_LIB_SUFFIX,  // lib*.so output file
    GB_jit_cache_path, kernel_name, GB_OBJ_SUFFIX,  // *.o input file
    GB_jit_C_libraries) ;           // libraries to link with

    GBURBLE ("(jit compile: %s) ", GB_jit_command) ;

    // compile the library and return result
    int result = system (GB_jit_command) ;
    GBURBLE ("(jit result: %d) ", result) ;
    return (result) ;
}

#endif

//------------------------------------------------------------------------------
// GB_jitifyer_hash:  compute the hash
//------------------------------------------------------------------------------

// xxHash uses switch statements with no default case.
#pragma GCC diagnostic ignored "-Wswitch-default"

#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "xxhash.h"

// A hash value of zero is unique, and is used for all builtin operators and
// types to indicate that its hash value is not required.

// A hash value of UINT64_MAX is also special: it denotes an object that cannot
// be JIT'd.

// So in the nearly impossible case that XXH3_64bits returns a hash value that
// happens to be zero or UINT64_MAX, it is reset to GB_MAGIC instead.

uint64_t GB_jitifyer_hash_encoding
(
    GB_jit_encoding *encoding
)
{ 
    uint64_t hash ;
    hash = XXH3_64bits ((const void *) encoding, sizeof (GB_jit_encoding)) ;
    return ((hash == 0 || hash == UINT64_MAX) ? GB_MAGIC : hash) ;
}

uint64_t GB_jitifyer_hash
(
    const void *bytes,      // any string of bytes
    size_t nbytes,          // # of bytes to hash
    bool jitable            // true if the object can be JIT'd
)
{ 
    if (bytes == NULL || nbytes == 0) return (0) ;
    if (!jitable) return (UINT64_MAX) ;
    uint64_t hash ;
    hash = XXH3_64bits (bytes, nbytes) ;
    return ((hash == 0 || hash == UINT64_MAX) ? GB_MAGIC : hash) ;
}

