"""Adapt reviewed hosted Module/link count sites without changing pinned source."""


# Apply exactly reviewed source occurrences. source/changes are private strings
# and replacement triples; return adapted string or reject boundary drift.
def replace(source, changes):
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted module-count boundary drift: '+before)
        source = source.replace(before, after)
    return source


# source is final hosted link.c; convert Type increments/preflights only.
# Caller still excludes registry, allocation and publication mutations.
def adapt_link(source):
    changes = (
        ('Tmodlink.ref++;', 'dvm_runtime_type_retain(&Tmodlink.ref);', 1),
        ('t->ref++;', 'dvm_runtime_type_retain(&t->ref);', 1),
        ('Tmodlink.ref <= 0', 'dvm_refcount_load_i(&Tmodlink.ref) <= 0', 1),
        ('Tmodlink.ref == 0x7fffffff', 'dvm_refcount_load_i(&Tmodlink.ref) == 0x7fffffff', 1),
        ('t->ref <= 0', 'dvm_refcount_load_i(&t->ref) <= 0', 1),
        ('t->ref > 0x7ffffffe', 'dvm_refcount_load_i(&t->ref) > 0x7ffffffe', 1),
    )
    return '#include "type_counts.h"\n#include "disvm/refcount.h"\n'+replace(source, changes)


# source is final hosted load.c; preserve private Module initializations and
# exact unlink/MP/type cleanup after LAST. No post-nonfinal counter observation.
def adapt_load(source):
    changes = (
        ('m->ref++;', 'dvm_runtime_module_retain(&m->ref);', 1),
        ('m->ref--;\n\tif(m->ref > 0)\n\t\treturn;\n\tif(m->ref == -1)\n\t\tabort();',
         'if(!dvm_runtime_module_release(&m->ref))\n\t\treturn;', 1),
        ('h->t->ref++;', 'dvm_runtime_type_retain(&h->t->ref);', 1),
        ('pt->ref <= 0', 'dvm_refcount_load_i(&pt->ref) <= 0', 1),
        ('pt->ref > 0x7ffffffe', 'dvm_refcount_load_i(&pt->ref) > 0x7ffffffe', 1),
        ('Tarray.ref <= 0', 'dvm_refcount_load_i(&Tarray.ref) <= 0', 1),
        ('Tarray.ref == 0x7fffffff', 'dvm_refcount_load_i(&Tarray.ref) == 0x7fffffff', 1),
    )
    return '#include "module_counts.h"\n#include "type_counts.h"\n#include "disvm/refcount.h"\n'+replace(source, changes)


# source is final hosted xec.c after opcode-retain conversion. Only Module
# self-import owner/preflight change here; snapshots remain separately excluded.
def adapt_xec(source):
    changes = (
        ('m->ref++;', 'dvm_runtime_module_retain(&m->ref);', 1),
        ('m->ref <= 0', 'dvm_refcount_load_i(&m->ref) <= 0', 1),
        ('m->ref == 0x7fffffff', 'dvm_refcount_load_i(&m->ref) == 0x7fffffff', 1),
    )
    return '#include "module_counts.h"\n#include "disvm/refcount.h"\n'+replace(source, changes)
