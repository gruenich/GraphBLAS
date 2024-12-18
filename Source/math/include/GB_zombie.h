//------------------------------------------------------------------------------
// GB_zombie.h: definitions for zombies
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2025, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_ZOMBIE_H
#define GB_ZOMBIE_H

// An entry A(i,j) in a matrix can be marked as a "zombie".  A zombie is an
// entry that has been marked for deletion, but hasn't been deleted yet because
// it's more efficient to delete all zombies all at once, instead of one at a
// time.  Zombies are created by submatrix assignment, C(I,J)=A which copies
// not only new entries into C, but it also deletes entries already present in
// C.  If an entry appears in A but not C(I,J), it is a new entry; new entries
// placed in the pending tuple lists to be added later.  If an entry appear in
// C(I,J) but NOT in A, then it is marked for deletion by marking its row index
// as a zombie.

// Zombies can be restored as regular entries by GrB_*assign.  If an assignment
// C(I,J)=A finds an entry in A that is a zombie in C, the zombie becomes a
// regular entry, taking on the value from A.  The row index is 'dezombied'.

// Zombies are deleted and pending tuples are added into the matrix all at
// once, by GB_wait.

// For GraphBLAS 10.0.0 and later, the zombie function has changed to allow
// for a larger range of valid indices when using 32-bit integers, where now
// GB_ZOMBIE([0 1 2 3 ... INT32_MAX]) = [-1 -2 -3 ... INT32_MIN].  This allows
// the largest index of a 32-bit A->i array to be INT32_MAX, giving a maximum
// matrix dimension of exactly 2^31.

// Some algorithms need more space than this for their indices, at least
// temporarily.  GrB_mxm on the CPU uses a 4-state finite state machine held in
// the Hf array (not in C->i itself).  GrB_mxm on the GPU requires 4 bits for
// its buckets; for 32-bit matrices, the bucket assignments needs to be stored
// in a separate array.

// The max matrix dimensions for 64-bit integer matrices could be increased to
// to about 2^62 on the CPU.  This would still be OK for the Hf [hash] entries
// for the fine Hash method.  The GPU currently is using 4 bits for up to 16
// buckets ... but it is currently only using about 4 buckets.

#define GB_ZOMBIE(i)        (-(i)-1)
#define GB_DEZOMBIE(i)      (-(i)-1)
#define GB_IS_ZOMBIE(i)     ((i) < 0)
#define GB_UNZOMBIE(i)      (((i) < 0) ? GB_ZOMBIE (i) : (i))

#define GBI_UNZOMBIE(Ai,p,vlen) \
    ((Ai == NULL) ? ((p) % (vlen)) : GB_UNZOMBIE (Ai [p]))

#endif

