"""Convert reviewed explicit Heap/Type retains in the hosted opcode executor."""


# source is the final hosted xec.c view. Return adapted source or reject drift.
# Module counts, raw header snapshots and preflight observations stay under
# serialized admission. No source-slot, graph or JIT atomicity is inferred.
def adapt(source):
    changes = (
        ('h->ref++;', 'dvm_runtime_heap_retain(&h->ref);', 10),
        ('D2H(c)->ref++;', 'dvm_runtime_heap_retain(&D2H(c)->ref);', 1),
        ('b->t->ref++;', 'dvm_runtime_type_retain(&b->t->ref);', 1),
        ('t->ref++;', 'dvm_runtime_type_retain(&t->ref);', 2),
        ('Tlist.ref++;', 'dvm_runtime_type_retain(&Tlist.ref);', 1),
        ('Tptr.ref++;', 'dvm_runtime_type_retain(&Tptr.ref);', 1),
    )
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted opcode-count boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "heap_counts.h"\n#include "type_counts.h"\n'+source
