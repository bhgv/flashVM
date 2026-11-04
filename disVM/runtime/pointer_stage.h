/* Private native list-node construction state; not guest or serialized ABI. */
#ifndef DVM_POINTER_STAGE_H
#define DVM_POINTER_STAGE_H

struct List;
struct Heap;


/* Stable caller-owned staging survives protected VM errors; never copy/escape. */
typedef struct PointerBuild
{
    void *source; /* Borrowed source until checked retain becomes node owner. */
    struct List *tail; /* Independently live old tail; no destination publication. */
    struct Heap *node; /* Owned unpublished node, manually freed on stage failure. */
    int source_owner; /* Temporary source retain not yet installed. */
    int tail_owner; /* Temporary tail retain not yet linked. */
    int list_type; /* Temporary descriptor owner not yet installed. */
    int pointer_type; /* Temporary element descriptor owner not yet installed. */
    int rooted; /* Actual ordinary root through monitor callbacks. */
    int tail_linked; /* Node owns the acquired tail after explicit link. */
} PointerBuild;


/* [GC:] argument is stable PointerBuild, source/tail independently owned/rooted.
 * Allocate/retain a coherent unpublished node with tail=H; monitor may inspect/
 * raise but may not alter inputs/leave VM admission. Return1 or real VM error.
 * Caller handles rollback after failed kprotect. No object guards may be held. */
int dvm_runtime_pointer_stage(void *argument);


/* [GC:] Finish the staged tail owner after monitor success; publish nothing.
 * build is exclusive live staging; link/mark a genuine tail and track ownership.
 * Return void, no allocation/callback. Caller retains VM/world/source lifetime. */
void dvm_runtime_pointer_link(PointerBuild *build);


/* [GC:] Discard one partially/fully staged unpublished node after failed kprotect.
 * build owns all flagged resources, no external node owner exists. No consuming
 * monitor callback; native poolfree and nonfinal source/tail/type releases only.
 * Return void; invariant corruption aborts. Caller keeps original owners live. */
void dvm_runtime_pointer_rollback(PointerBuild *build);


/* Consume original destination owner nonfinally after a node owns its own tail.
 * tail is H or independently live Tlist. No callback/allocation; return void.
 * Requires serialized admission; missing original owner is a fatal invariant. */
void dvm_runtime_pointer_release_tail(struct List *tail);
#endif
