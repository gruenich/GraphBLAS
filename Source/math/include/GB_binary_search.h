//------------------------------------------------------------------------------
// GB_binary_search.h: binary search in a sorted list
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// DONE: 32/64 bit

#ifndef GB_BINARY_SEARCH_H
#define GB_BINARY_SEARCH_H

//------------------------------------------------------------------------------
// GB_TRIM_BINARY_SEARCH: simple binary search
//------------------------------------------------------------------------------

// search for integer i in the list X [pleft...pright]; no zombies.
// The list X [pleft ... pright] is in ascending order.  It may have
// duplicates.

#ifdef GB_CUDA_KERNEL

    // binary search on the GPU, with fewer branches
    #define GB_TRIM_BINARY_SEARCH(i,X,pleft,pright)                         \
    {                                                                       \
        /* binary search of X [pleft ... pright] for integer i */           \
        while (pleft < pright)                                              \
        {                                                                   \
            int64_t pmiddle = (pleft + pright) >> 1 ;                       \
            bool less = (X [pmiddle] < i) ;                                 \
            pleft  = less ? (pmiddle+1) : pleft ;                           \
            pright = less ? pright : pmiddle ;                              \
        }                                                                   \
        /* binary search is narrowed down to a single item */               \
        /* or it has found the list is empty */                             \
        /* ASSERT (pleft == pright || pleft == pright + 1) ; */             \
    }

#else

    // binary search on the CPU
    #define GB_TRIM_BINARY_SEARCH(i,X,pleft,pright)                         \
    {                                                                       \
        /* binary search of X [pleft ... pright] for integer i */           \
        while (pleft < pright)                                              \
        {                                                                   \
            int64_t pmiddle = (pleft + pright) / 2 ;                        \
            if (X [pmiddle] < i)                                            \
            {                                                               \
                /* if in the list, it appears in [pmiddle+1..pright] */     \
                pleft = pmiddle + 1 ;                                       \
            }                                                               \
            else                                                            \
            {                                                               \
                /* if in the list, it appears in [pleft..pmiddle] */        \
                pright = pmiddle ;                                          \
            }                                                               \
        }                                                                   \
        /* binary search is narrowed down to a single item */               \
        /* or it has found the list is empty */                             \
        /* ASSERT (pleft == pright || pleft == pright + 1) ; */             \
    }

#endif

GB_STATIC_INLINE void GB_trim_binary_search_32
(
    const uint32_t i,           // item to look for
    const uint32_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    // binary search of X [pleft ... pright] for the integer i
    while (*pleft < *pright)
    {
        #ifdef CUDA_KERNEL
            int64_t pmiddle = (*pleft + *pright) >> 1 ;
            bool less = (X [pmiddle] < i) ;
            *pleft  = less ? (pmiddle+1) : *pleft ;
            *pright = less ? *pright : pmiddle ;
        #else
            int64_t pmiddle = (*pleft + *pright) / 2 ;
            if (X [pmiddle] < i)
            {
                // if in the list, it appears in [pmiddle+1..pright]
                *pleft = pmiddle + 1 ;
            }
            else
            {
                // if in the list, it appears in [pleft..pmiddle]
                *pright = pmiddle ;
            }
        #endif
    }
    // binary search is narrowed down to a single item
    // or it has found the list is empty
    ASSERT (*pleft == *pright || *pleft == *pright + 1) ;
}

GB_STATIC_INLINE void GB_trim_binary_search_64
(
    const uint64_t i,           // item to look for
    const uint64_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    // binary search of X [pleft ... pright] for the integer i
    while (*pleft < *pright)
    {
        #ifdef CUDA_KERNEL
            int64_t pmiddle = (*pleft + *pright) >> 1 ;
            bool less = (X [pmiddle] < i) ;
            *pleft  = less ? (pmiddle+1) : *pleft ;
            *pright = less ? *pright : pmiddle ;
        #else
            int64_t pmiddle = (*pleft + *pright) / 2 ;
            if (X [pmiddle] < i)
            {
                // if in the list, it appears in [pmiddle+1..pright]
                *pleft = pmiddle + 1 ;
            }
            else
            {
                // if in the list, it appears in [pleft..pmiddle]
                *pright = pmiddle ;
            }
        #endif
    }
    // binary search is narrowed down to a single item
    // or it has found the list is empty
    ASSERT (*pleft == *pright || *pleft == *pright + 1) ;
}

GB_STATIC_INLINE void GB_trim_binary_search
(
    const uint64_t i,           // item to look for
    const void *X,              // array to search; no zombies
    const bool X_is_32,         // if true, X is 32-bit, else 64
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    if (X_is_32)
    {
        GB_trim_binary_search_32 (i, (const uint32_t *) X, pleft, pright) ;
    }
    else
    {
        GB_trim_binary_search_64 (i, (const uint64_t *) X, pleft, pright) ;
    }
}

//------------------------------------------------------------------------------
// GB_BINARY_SEARCH: binary search and check if found
//------------------------------------------------------------------------------

// If found is true then X [pleft == pright] == i.  If duplicates appear then
// X [pleft] is any one of the entries with value i in the list.
// If found is false then
//    X [original_pleft ... pleft-1] < i and
//    X [pleft+1 ... original_pright] > i holds.
// The value X [pleft] may be either < or > i.

#define GB_BINARY_SEARCH(i,X,pleft,pright,found)                            \
{                                                                           \
    GB_TRIM_BINARY_SEARCH (i, X, pleft, pright) ;                           \
    found = (pleft == pright && X [pleft] == i) ;                           \
}

GB_STATIC_INLINE bool GB_binary_search_32
(
    const uint32_t i,           // item to look for
    const uint32_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    GB_trim_binary_search_32 (i, X, pleft, pright) ;
    return (*pleft == *pright && X [*pleft] == i) ;
}

GB_STATIC_INLINE bool GB_binary_search_64
(
    const uint64_t i,           // item to look for
    const uint64_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    GB_trim_binary_search_64 (i, X, pleft, pright) ;
    return (*pleft == *pright && X [*pleft] == i) ;
}

GB_STATIC_INLINE bool GB_binary_search
(
    const uint64_t i,           // item to look for
    const void *X,              // array to search; no zombies
    const bool X_is_32,         // if true, X is 32-bit, else 64
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    if (X_is_32)
    {
        return (GB_binary_search_32 (i, (const uint32_t *) X, pleft, pright)) ;
    }
    else
    {
        return (GB_binary_search_64 (i, (const uint64_t *) X, pleft, pright)) ;
    }
}

//------------------------------------------------------------------------------
// GB_SPLIT_BINARY_SEARCH: binary search, and then partition the list
//------------------------------------------------------------------------------

// If found is true then X [pleft] == i.  If duplicates appear then X [pleft]
//    is any one of the entries with value i in the list.
// If found is false then
//    X [original_pleft ... pleft-1] < i and
//    X [pleft ... original_pright] > i holds, and pleft-1 == pright
// If X has no duplicates, then whether or not i is found,
//    X [original_pleft ... pleft-1] < i and
//    X [pleft ... original_pright] >= i holds.

#define GB_SPLIT_BINARY_SEARCH(i,X,pleft,pright,found)                      \
{                                                                           \
    GB_BINARY_SEARCH (i, X, pleft, pright, found)                           \
    if (!found && (pleft == pright))                                        \
    {                                                                       \
        if (i > X [pleft])                                                  \
        {                                                                   \
            pleft++ ;                                                       \
        }                                                                   \
        else                                                                \
        {                                                                   \
            pright++ ;                                                      \
        }                                                                   \
    }                                                                       \
}

GB_STATIC_INLINE bool GB_split_binary_search_32
(
    const uint32_t i,           // item to look for
    const uint32_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    bool found = GB_binary_search_32 (i, X, pleft, pright) ;
    if (!found && (*pleft == *pright))
    {
        if (i > X [*pleft])
        {
            (*pleft)++ ;
        }
        else
        {
            (*pright)++ ;
        }
    }
    return (found) ;
}

GB_STATIC_INLINE bool GB_split_binary_search_64
(
    const uint64_t i,           // item to look for
    const uint64_t *restrict X, // array to search; no zombies
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    bool found = GB_binary_search_64 (i, X, pleft, pright) ;
    if (!found && (*pleft == *pright))
    {
        if (i > X [*pleft])
        {
            (*pleft)++ ;
        }
        else
        {
            (*pright)++ ;
        }
    }
    return (found) ;
}

GB_STATIC_INLINE bool GB_split_binary_search
(
    const uint64_t i,           // item to look for
    const void *X,              // array to search; no zombies
    const bool X_is_32,         // if true, X is 32-bit, else 64
    int64_t *pleft,             // look in X [pleft:pright]
    int64_t *pright
)
{
    if (X_is_32)
    {
        return (GB_binary_search_32 (i, (const uint32_t *) X, pleft, pright)) ;
    }
    else
    {
        return (GB_binary_search_64 (i, (const uint64_t *) X, pleft, pright)) ;
    }
}

//------------------------------------------------------------------------------
// GB_TRIM_BINARY_SEARCH_ZOMBIE: binary search in the presence of zombies
//------------------------------------------------------------------------------

#define GB_TRIM_BINARY_SEARCH_ZOMBIE(i,X,pleft,pright)                      \
{                                                                           \
    /* binary search of X [pleft ... pright] for integer i */               \
    while (pleft < pright)                                                  \
    {                                                                       \
        int64_t pmiddle = (pleft + pright) / 2 ;                            \
        if (i > GB_UNZOMBIE (X [pmiddle]))                                  \
        {                                                                   \
            /* if in the list, it appears in [pmiddle+1..pright] */         \
            pleft = pmiddle + 1 ;                                           \
        }                                                                   \
        else                                                                \
        {                                                                   \
            /* if in the list, it appears in [pleft..pmiddle] */            \
            pright = pmiddle ;                                              \
        }                                                                   \
    }                                                                       \
    /* binary search is narrowed down to a single item */                   \
    /* or it has found the list is empty */                                 \
    /* ASSERT (pleft == pright || pleft == pright + 1) ; */                 \
}

//------------------------------------------------------------------------------
// GB_BINARY_SEARCH_ZOMBIE: binary search with zombies; check if found
//------------------------------------------------------------------------------

#define GB_BINARY_SEARCH_ZOMBIE(i,X,pleft,pright,found,nzombies,is_zombie)  \
{                                                                           \
    if (nzombies > 0)                                                       \
    {                                                                       \
        GB_TRIM_BINARY_SEARCH_ZOMBIE (i, X, pleft, pright) ;                \
        found = false ;                                                     \
        is_zombie = false ;                                                 \
        if (pleft == pright)                                                \
        {                                                                   \
            int64_t i2 = X [pleft] ;                                        \
            is_zombie = GB_IS_ZOMBIE (i2) ;                                 \
            if (is_zombie)                                                  \
            {                                                               \
                i2 = GB_DEZOMBIE (i2) ;                                     \
            }                                                               \
            found = (i == i2) ;                                             \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        is_zombie = false ;                                                 \
        GB_BINARY_SEARCH (i, X, pleft, pright, found)                       \
    }                                                                       \
}

//------------------------------------------------------------------------------
// GB_SPLIT_BINARY_SEARCH_ZOMBIE: binary search with zombies; then partition
//------------------------------------------------------------------------------

#define GB_SPLIT_BINARY_SEARCH_ZOMBIE(i,X,pleft,pright,found,nzom,is_zombie) \
{                                                                           \
    if (nzom > 0)                                                           \
    {                                                                       \
        GB_TRIM_BINARY_SEARCH_ZOMBIE (i, X, pleft, pright) ;                \
        found = false ;                                                     \
        is_zombie = false ;                                                 \
        if (pleft == pright)                                                \
        {                                                                   \
            int64_t i2 = X [pleft] ;                                        \
            is_zombie = GB_IS_ZOMBIE (i2) ;                                 \
            if (is_zombie)                                                  \
            {                                                               \
                i2 = GB_DEZOMBIE (i2) ;                                     \
            }                                                               \
            found = (i == i2) ;                                             \
            if (!found)                                                     \
            {                                                               \
                if (i > i2)                                                 \
                {                                                           \
                    pleft++ ;                                               \
                }                                                           \
                else                                                        \
                {                                                           \
                    pright++ ;                                              \
                }                                                           \
            }                                                               \
        }                                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        is_zombie = false ;                                                 \
        GB_SPLIT_BINARY_SEARCH (i, X, pleft, pright, found)                 \
    }                                                                       \
}

#endif

