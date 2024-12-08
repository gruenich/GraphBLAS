//------------------------------------------------------------------------------
// GB_where.h: definitions for Werk space and error logging
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_WHERE_H
#define GB_WHERE_H

//------------------------------------------------------------------------------
// GB_WHERE*: allocate the Werk stack and enable error logging
//------------------------------------------------------------------------------

// GB_WHERE keeps track of the currently running user-callable function.
// User-callable functions in this implementation are written so that they do
// not call other unrelated user-callable functions (except for GrB_*free).
// Related user-callable functions can call each other since they all report
// the same type-generic name.  Internal functions can be called by many
// different user-callable functions, directly or indirectly.  It would not be
// helpful to report the name of an internal function that flagged an error
// condition.  Thus, each time a user-callable function is entered, it logs the
// name of the function with the GB_WHERE macro.

#define GB_WERK(where_string)                                       \
    /* construct the Werk */                                        \
    GB_Werk_struct Werk_struct ;                                    \
    GB_Werk Werk = GB_Werk_init (&Werk_struct, where_string) ;

// initialize the Werk object
static inline GB_Werk GB_Werk_init (GB_Werk Werk, const char *where_string)
{
    // set Werk->where so GrB_error can report it if needed
    Werk->where = where_string ;

    // get the pointer to where any error will be logged
    Werk->logger_handle = NULL ;
    Werk->logger_size_handle = NULL ;

    // initialize the Werk stack
    Werk->pwerk = 0 ;

    // initialize the global and matrix integer control
    Werk->global_p_control = GB_Global_p_control_get ( ) ;
    Werk->global_i_control = GB_Global_i_control_get ( ) ;
    Werk->matrix_p_control = Werk->global_p_control ;
    Werk->matrix_i_control = Werk->global_i_control ;

    // return result
    return (Werk) ;
}

// return true if a matrix is valid, based on the current control
static inline bool GB_valid_integers
(
    GrB_Matrix C,
    GB_Werk Werk
)
{
    // a NULL matrix has no integers
    if (C == NULL) return (true) ;

    // a full or bitmap matrix has no integers
    if ((C->p == NULL && C->h == NULL &&
         C->i == NULL && C->Y == NULL)) return (true) ;

    const bool p_is_32 = C->p_is_32 ;
    const bool i_is_32 = C->i_is_32 ;

    // check the global pi_controls
    if (!GB_valid_control (Werk->global_p_control, p_is_32)) return (false) ;
    if (!GB_valid_control (Werk->global_i_control, i_is_32)) return (false) ;

    // check the matrix pi_controls
    if (!GB_valid_control (C->p_control, p_is_32)) return (false) ;
    if (!GB_valid_control (C->i_control, i_is_32)) return (false) ;

    // assert that the matrix status is large enough for its content
    #ifdef GB_DEBUG
    ASSERT (!A->p_is_32 || GB_validate_p_is_32 (true, A->nvals)) ;
    ASSERT (!A->i_is_32 || GB_validate_i_is_32 (true, A->vlen, A->vdim)) ;
    #endif
}

// C is a matrix, vector, or scalar
#define GB_WHERE(C,where_string)                                    \
    if (!GB_Global_GrB_init_called_get ( ))                         \
    {                                                               \
        return (GrB_PANIC) ; /* GrB_init not called */              \
    }                                                               \
    GB_WERK (where_string)                                          \
    if (C != NULL)                                                  \
    {                                                               \
        /* free any prior error logged in the object */             \
        GB_FREE (&(C->logger), C->logger_size) ;                    \
        /* ensure the matrix has valid integers */                  \
        if (!GB_valid_integers (C, Werk))                           \
        {                                                           \
            return (GrB_INVALID_OBJECT) ;                           \
        }                                                           \
        /* get the error logger */                                  \
        Werk->logger_handle = &(C->logger) ;                        \
        Werk->logger_size_handle = &(C->logger_size) ;              \
        /* get the matrix p_control and i_control */                \
        Werk->matrix_p_control = C->p_control ;                     \
        Werk->matrix_i_control = C->i_control ;                     \
    }

// for descriptors
#define GB_WHERE_DESC(desc,where_string)                            \
    if (!GB_Global_GrB_init_called_get ( ))                         \
    {                                                               \
        return (GrB_PANIC) ; /* GrB_init not called */              \
    }                                                               \
    GB_WERK (where_string)                                          \
    if (desc != NULL)                                               \
    {                                                               \
        /* free any prior error logged in the object */             \
        GB_FREE (&(desc->logger), desc->logger_size) ;              \
        Werk->logger_handle = &(desc->logger) ;                     \
        Werk->logger_size_handle = &(desc->logger_size) ;           \
    }

// create the Werk, with no error logging
#define GB_WHERE1(where_string)                                     \
    if (!GB_Global_GrB_init_called_get ( ))                         \
    {                                                               \
        return (GrB_PANIC) ; /* GrB_init not called */              \
    }                                                               \
    GB_WERK (where_string)

//------------------------------------------------------------------------------
// GB_ERROR: error logging
//------------------------------------------------------------------------------

// The GB_ERROR macro logs an error in the logger error string.
//
//  if (i >= nrows)
//  {
//      GB_ERROR (GrB_INDEX_OUT_OF_BOUNDS,
//          "Row index %d out of bounds; must be < %d", i, nrows) ;
//  }
//
// The user can then do:
//
//  const char *error ;
//  GrB_error (&error, A) ;
//  printf ("%s", error) ;

const char *GB_status_code (GrB_Info info) ;

// maximum size of the error logger string
#define GB_LOGGER_LEN 384

// log an error in the error logger string and return the error
#define GB_ERROR(info,format,...)                                           \
{                                                                           \
    if (Werk != NULL)                                                       \
    {                                                                       \
        char **logger_handle = Werk->logger_handle ;                        \
        if (logger_handle != NULL)                                          \
        {                                                                   \
            size_t *logger_size_handle = Werk->logger_size_handle ;         \
            (*logger_handle) = GB_CALLOC (GB_LOGGER_LEN+1, char,            \
                logger_size_handle) ;                                       \
            if ((*logger_handle) != NULL)                                   \
            {                                                               \
                snprintf ((*logger_handle), GB_LOGGER_LEN,                  \
                    "GraphBLAS error: %s\nfunction: %s\n" format,           \
                    GB_status_code (info), Werk->where, __VA_ARGS__) ;      \
            }                                                               \
        }                                                                   \
    }                                                                       \
    return (info) ;                                                         \
}

#endif

