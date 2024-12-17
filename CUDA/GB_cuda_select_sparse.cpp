
#include "GB_cuda_select.hpp"

#undef GB_FREE_ALL
#define GB_FREE_ALL         \
{                           \
    GB_phybix_free (C) ;    \
}

#define BLOCK_SIZE 512
#define LOG2_BLOCK_SIZE 9

GrB_Info GB_cuda_select_sparse
(
    GrB_Matrix C,
    const bool C_iso,
    const GrB_IndexUnaryOp op,
    const bool flipij,
    const GrB_Matrix A,
    const GB_void *athunk,
    const GB_void *ythunk
)
{
    // check inputs
    ASSERT (C != NULL && !(C->static_header)) ;
    ASSERT (A != NULL && !(A->static_header)) ;

    GrB_Info info = GrB_NO_VALUE ;

    // FIXME: use the stream pool
    cudaStream_t stream ;
    CUDA_OK (cudaStreamCreate (&stream)) ;

    GrB_Index anz = GB_nnz_held (A) ;

    int32_t number_of_sms = GB_Global_gpu_sm_get (0) ;
    int64_t raw_gridsz = GB_ICEIL (anz, BLOCK_SIZE) ;
    int32_t gridsz = std::min (raw_gridsz, (int64_t) (number_of_sms * 256)) ;
    gridsz = std::max (gridsz, 1) ;

    // Initialize C to be a user-returnable hypersparse empty matrix.
    // If needed, we handle the hyper->sparse conversion below.
    GB_OK (GB_new (&C, A->type, A->vlen, A->vdim, GB_ph_calloc, A->is_csc,
            GxB_HYPERSPARSE, A->hyper_switch, /* C->plen: */ 1,
            /* FIXME: */ false, false)) ;
    C->jumbled = A->jumbled ;
    C->iso = C_iso ;

    info = GB_cuda_select_sparse_jit (C, A,
        flipij, ythunk, op, stream, gridsz, BLOCK_SIZE) ;
//  printf ("cuda select sparse jit, info %d iso %d\n", info, C_iso) ;

    CUDA_OK (cudaStreamSynchronize (stream)) ;
    CUDA_OK (cudaStreamDestroy (stream)) ;

    ASSERT (C->x != NULL) ;

    GB_OK (info) ;

    if (C_iso)
    {
        // If C is iso, initialize the iso entry
        GB_select_iso ((GB_void *) C->x, op->opcode, athunk,
            (GB_void *) A->x, A->type->size) ;
    }

    if (C->nvec == C->vdim)
    {
        // C hypersparse with all vectors present; quick convert to sparse
        GB_FREE (&(C->h), C->h_size) ;
    }

    ASSERT_MATRIX_OK (C, "C output of cuda_select_sparse", GB0) ;
    return GrB_SUCCESS ;
}

