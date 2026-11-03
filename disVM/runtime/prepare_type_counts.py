"""Adapt only hosted heap Type counts; refuse changed pinned source boundaries."""


# source is the private hosted heap source string. Return its reviewed count
# adaptation or raise on drift. Heap.ref, initialization and other C/JIT bodies
# remain untouched; this transformation grants no parallel heap admission.
def adapt(source):
    changes = (
        ('if(t->ref-- == 1)', 'if(dvm_runtime_type_release(&t->ref))', 1),
        ('t->ref--;\n\t\tif(t->ref == 0)', 'if(dvm_runtime_type_release(&t->ref))', 1),
        ('t->ref--;\n\t\t\tif(t->ref == 0)', 'if(dvm_runtime_type_release(&t->ref))', 1),
        ('th->t->ref--;', 'dvm_runtime_type_release_nonfinal(&th->t->ref);', 1),
        ('t == nil || --t->ref > 0', 't == nil || !dvm_runtime_type_release(&t->ref)', 1),
        ('t->ref++;', 'dvm_runtime_type_retain(&t->ref);', 3),
        ('Tarray.ref++;', 'dvm_runtime_type_retain(&Tarray.ref);', 1),
        ('t->ref <= 0', 'dvm_refcount_load_i(&t->ref) <= 0', 3),
        ('t->ref == 0x7fffffff', 'dvm_refcount_load_i(&t->ref) == 0x7fffffff', 2),
        ('t->ref > 0x7ffffffe', 'dvm_refcount_load_i(&t->ref) > 0x7ffffffe', 1),
        ('Tarray.ref <= 0', 'dvm_refcount_load_i(&Tarray.ref) <= 0', 1),
        ('Tarray.ref == 0x7fffffff', 'dvm_refcount_load_i(&Tarray.ref) == 0x7fffffff', 1),
    )
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted Type-count boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "type_counts.h"\n#include "disvm/refcount.h"\n'+source
