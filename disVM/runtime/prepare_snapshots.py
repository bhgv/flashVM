"""Prepare count-safe private header observations, not parallel field mutation."""
import re


# Match only whole Heap-header reads in reviewed C view. Preserve indentation,
# reject missing/extra sites. source is private string, expected is exact count.
# Scalar/payload copies remain unchanged. Caller supplies protected source spans.
def headers(source, expected):
    pattern = r'(?m)^([ \t]*)memmove\(&(\w+), ([^\n;]+)-sizeof\(Heap\), sizeof\(\2\)\);$'


    # match borrows one reviewed header read; return a guarded C block, keeping
    # the private output unchanged on helper refusal and raising the VM error.
    def replace(match):
        indent, name, address = match.groups()
        return (indent+'if(!dvm_runtime_heap_snapshot(&'+name+', '+address+'-sizeof(Heap)))\n'+
                indent+'{\n'+indent+'\terror(exHeap);\n'+indent+'}')
    result, count = re.subn(pattern, replace, source)
    if count != expected:
        raise ValueError('hosted Heap snapshot boundary drift: '+str(count))
    return '#include "heap_snapshot.h"\n'+result


# Final hosted string.c has7 header snapshots,18 direct Heap observations,
# two Tstring retains/four preflights and one validated Type-count observation.
# Snapshot-local fields remain plain PRIVATE observations, not shared storage.
def adapt_string(source):
    source = headers(source, 7)
    changes = (
        ('D2H(old)->ref', 'dvm_refcount_load_u(&D2H(old)->ref)', 14),
        ('D2H(ds)->ref', 'dvm_refcount_load_u(&D2H(ds)->ref)', 1),
        ('D2H(s1)->ref', 'dvm_refcount_load_u(&D2H(s1)->ref)', 2),
        ('D2H(s2)->ref', 'dvm_refcount_load_u(&D2H(s2)->ref)', 1),
        ('Tstring.ref++;', 'dvm_runtime_type_retain(&Tstring.ref);', 2),
        ('Tstring.ref <= 0', 'dvm_refcount_load_i(&Tstring.ref) <= 0', 2),
        ('Tstring.ref == 0x7fffffff', 'dvm_refcount_load_i(&Tstring.ref) == 0x7fffffff', 2),
        ('memmove(&references, (uchar*)target.t+offsetof(Type, ref), sizeof(references));',
         'references = dvm_refcount_load_i(&target.t->ref);', 1),
    )
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted string-count boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "type_counts.h"\n#include "disvm/refcount.h"\n'+source


# Final hosted xec.c has6 whole-header observations; keep other preflights,
# metadata publication and emitted JIT paths explicitly excluded.
def adapt_xec(source):
    return headers(source, 6)
