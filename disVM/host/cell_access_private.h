/* Shared private domain geometry; never a serialized/public Heap layout. */
#ifndef DVM_CELL_ACCESS_PRIVATE_H
#define DVM_CELL_ACCESS_PRIVATE_H
#include "disvm/cell_access.h"

#define DVM_CELL_STRIPES 256 /* Collision exclusion, not an object/thread quota. */


/* Creator and each complete/partial native access hold independent borrows. */
struct DVM_CellDomain
{
    unsigned long references; /* Checked native count; caller protects first lookup. */
    DVM_ObjectDomain *parking; /* Owned shared parking metadata. */
    DVM_ObjectGuard stripes[DVM_CELL_STRIPES]; /* Stable native guard addresses. */
};
#endif
