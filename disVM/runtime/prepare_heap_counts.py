"""Convert only reviewed hosted heap.c Heap counts, preserving GC policy."""


# source is a private hosted heap view after Type adaptation and RC queue hook.
# Return changed owned string or reject pinned boundary drift. No wire/layout
# change and no inference that remaining C/JIT counters are converted.
def adapt(source):
    changes = (
        ('if(--h->ref > 0 || gchalt > 64)', 'if(!dvm_runtime_heap_release(&h->ref) || gchalt > 64)', 1),
        ('if(th->ref-- != 1)', 'if(!dvm_runtime_heap_release(&th->ref))', 1),
        ('h->ref++;', 'dvm_runtime_heap_retain(&h->ref);', 3),
        ('return D2H(v)->ref;', 'return dvm_refcount_load_u(&D2H(v)->ref);', 1),
        ('D2H(sa->root)->ref == 0', 'dvm_refcount_load_u(&D2H(sa->root)->ref) == 0', 1),
        ('D2H(sa)->ref == 0', 'dvm_refcount_load_u(&D2H(sa)->ref) == 0', 1),
        ('h->ref == 0', 'dvm_refcount_load_u(&h->ref) == 0', 1),
        ('h->ref == (ulong)~0', 'dvm_refcount_load_u(&h->ref) == (ulong)~0', 1),
    )
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted Heap-count boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "heap_counts.h"\n'+source
