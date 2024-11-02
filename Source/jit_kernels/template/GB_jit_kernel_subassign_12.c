//------------------------------------------------------------------------------
// GB_jit_kernel_subassign_12.c: C(I,J)<M or !M,repl> += A ; using S
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2024, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

GB_JIT_GLOBAL GB_JIT_KERNEL_SUBASSIGN_PROTO (GB_jit_kernel) ;
GB_JIT_GLOBAL GB_JIT_KERNEL_SUBASSIGN_PROTO (GB_jit_kernel)
{
    // get callback functions
//  GB_GET_CALLBACK (GB_AxB_saxpy3_cumsum) ;
//  GB_GET_CALLBACK (GB_bitmap_M_scatter) ;
//  GB_GET_CALLBACK (GB_bitmap_M_scatter_whole) ;
//  GB_GET_CALLBACK (GB_bix_alloc) ;
//  GB_GET_CALLBACK (GB_ek_slice) ;
//  GB_GET_CALLBACK (GB_ek_slice_merge1) ;
    GB_GET_CALLBACK (GB_free_memory) ;
//  GB_GET_CALLBACK (GB_malloc_memory) ;
//  GB_GET_CALLBACK (GB_memset) ;
//  GB_GET_CALLBACK (GB_qsort_1) ;
    GB_GET_CALLBACK (GB_werk_pop) ;
    GB_GET_CALLBACK (GB_werk_push) ;

    GB_GET_CALLBACK (GB_hyper_hash_build) ;
//  GB_GET_CALLBACK (GB_subassign_one_slice) ;
    GB_GET_CALLBACK (GB_add_phase0) ;
    GB_GET_CALLBACK (GB_ewise_slice) ;
    GB_GET_CALLBACK (GB_subassign_IxJ_slice) ;
    GB_GET_CALLBACK (GB_Pending_ensure) ;
//  GB_GET_CALLBACK (GB_subassign_08n_slice) ;

    #include "template/GB_subassign_12_template.c"
}

