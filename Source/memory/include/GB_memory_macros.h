//------------------------------------------------------------------------------
// GB_memory_macros.h: memory allocation macros
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_MEMORY_MACROS_H
#define GB_MEMORY_MACROS_H

//------------------------------------------------------------------------------
// malloc/calloc/realloc/free: for permanent contents of GraphBLAS objects
//------------------------------------------------------------------------------

// FIXME: revise the memory macros:

#if 0

    with memdump
    #define GB_MALLOC(n,sizeof_type,s)
        GB_malloc_memory (n, sizeof_type, s, __FILE__, __LINE__)

    #define GB_REALLOC(p,n,sizeof_type,s,ok)
        GB_realloc_memory ((void *) p, n, sizeof_type, s, ok,
            __FILE__, __LINE__)

    normal usage
    #define GB_MALLOC(n,sizeof_type,s)
        GB_malloc_memory (n, sizeof_type, s)

    #define GB_REALLOC(p,n,sizeof_type,s,ok)
        GB_realloc_memory ((void *) p, n, sizeof_type, s, ok)

#endif

#ifdef GB_MEMDUMP

    #define GB_FREE(p,s)                                                    \
    {                                                                       \
        if (p != NULL && (*(p)) != NULL)                                    \
        {                                                                   \
            printf ("free    %p %8ld: (%s, line %d)\n", /* MEMDUMP */       \
                (void *) (*p), s, __FILE__, __LINE__) ;                     \
        }                                                                   \
        GB_free_memory ((void **) p, s) ;                                   \
    }

    #define GB_CALLOC(n,type,s)                                             \
        (type *) GB_calloc_memory (n, sizeof (type), s) ;                   \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */

    #define GB_MALLOC(n,type,s)                                             \
        (type *) GB_malloc_memory (n, sizeof (type), s) ;                   \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */

    #define GB_MALLOC_MEMORY(n,sizeof_type,s)                               \
        GB_malloc_memory (n, sizeof_type, s) ;                              \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */

    #define GB_CALLOC_MEMORY(n,sizeof_type,s)                               \
        GB_calloc_memory (n, sizeof_type, s) ;                              \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */

    #define GB_REALLOC(p,nnew,type,s,ok)                                    \
    {                                                                       \
        p = (type *) GB_realloc_memory (nnew, sizeof (type),                \
            (void *) p, s, ok) ;                                            \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */ ;      \
    }

    #define GB_REALLOC_MEMORY(p,nnew,sizeof_type,s,ok)                      \
    {                                                                       \
        p = GB_realloc_memory (nnew, sizeof_type,                           \
            (void *) p, s, ok) ;                                            \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */ ;      \
    }

    #define GB_XALLOC(use_calloc,iso,n,type_size,s)                         \
        GB_xalloc_memory (use_calloc, iso, n, type_size, s) ;               \
        printf ("(%s, line %d)\n", __FILE__, __LINE__) /* MEMDUMP */

#else

    #define GB_FREE(p,s)                                                    \
        GB_free_memory ((void **) p, s)

    #define GB_CALLOC(n,type,s)                                             \
        (type *) GB_calloc_memory (n, sizeof (type), s)

    #define GB_MALLOC(n,type,s)                                             \
        (type *) GB_malloc_memory (n, sizeof (type), s)

    #define GB_MALLOC_MEMORY(n,sizeof_type,s)                               \
        GB_malloc_memory (n, sizeof_type, s)

    #define GB_CALLOC_MEMORY(n,sizeof_type,s)                               \
        GB_calloc_memory (n, sizeof_type, s)

    #define GB_REALLOC(p,nnew,type,s,ok)                                    \
    {                                                                       \
        p = (type *) GB_realloc_memory (nnew, sizeof (type),                \
            (void *) p, s, ok) ;                                            \
    }

    #define GB_REALLOC_MEMORY(p,nnew,sizeof_type,s,ok)                      \
    {                                                                       \
        p = GB_realloc_memory (nnew, sizeof_type, (void *) p, s, ok) ;      \
    }

    #define GB_XALLOC(use_calloc,iso,n,type_size,s)                         \
        GB_xalloc_memory (use_calloc, iso, n, type_size, s)

#endif

//------------------------------------------------------------------------------
// malloc/calloc/realloc/free: for workspace
//------------------------------------------------------------------------------

// FIXME: remove these

// These macros currently do the same thing as the 4 macros above, but that may
// change in the future.  Even if they always do the same thing, it's useful to
// tag the source code for the allocation of workspace differently from the
// allocation of permament space for a GraphBLAS object, such as a GrB_Matrix.

#define GB_CALLOC_WORK(n,type,s) GB_CALLOC(n,type,s)
#define GB_MALLOC_WORK(n,type,s) GB_MALLOC(n,type,s)
#define GB_REALLOC_WORK(p,nnew,type,s,ok) GB_REALLOC(p,nnew,type,s,ok)
#define GB_FREE_WORK(p,s) GB_FREE(p,s)

#endif

