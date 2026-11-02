/* Internal layout embedded in native_context; callers use the opaque API. */
#ifndef DISVM_NATIVE_OWNER_PRIVATE_H
#define DISVM_NATIVE_OWNER_PRIVATE_H
#include "disvm/native_owner.h"


/* No managed pointers: mutable even during native waits, private to one owner. */
struct DVM_HeapOwner
{
    DVM_NativeWorld *world;          /* Borrowed until explicit idle detach. */
    DVM_WorldParticipant *roots;     /* Stable registration, not a managed Prog owner. */
    DVM_HeapPhase phase;             /* Current owner-only lifecycle state. */
    unsigned long before;           /* Collector epoch captured before releasing. */
    unsigned long releases;         /* Owner-private native suspend count. */
    unsigned long reentries;        /* Owner-private matched resume count. */
    unsigned long gc_progress;      /* Owner-private accumulated epoch delta. */
    int pending_progress;           /* Resumed; caller has not yet sampled epoch. */
};
#endif
