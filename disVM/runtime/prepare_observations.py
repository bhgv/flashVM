# Copyright (c) 2026 bhgv
# [GC:] EN: Adapt native count observations and owner register publications.
# RU: Адаптация чтения native-счётчиков и публикации регистров владельца.
"""Convert remaining reviewed direct observations and paired C-call counts."""

import re
from prepare_snapshots import headers


# Replace exact checked source occurrences; source/changes are private strings
# and triples. Reject drift rather than leaving a field operation unconverted.
def replacements(source, changes):
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError("count-observation boundary drift: " + before)
        source = source.replace(before, after)
    return source


# Observe one header field without reading its remaining bytes. destination,
# address and failure are trusted local C expressions; return guarded C block.
def observe(destination, address, failure="error(exHeap);"):
    return (
        "if(!dvm_runtime_heap_count(" + address + ", &" + destination + ")) {\n"
        "\t\t" + failure + "\n\t}"
    )


# Final hosted executor has 25 direct Heap/20 Type comparisons. Snapshot-local
# members stay private. The paired MCALL release preserves legacy deferred-zero
# semantics: it consumes owner but invokes no destructor, even on LAST.
def adapt_xec(source):
    fields = (
        ("h->ref", 20, "u"),
        ("type->ref", 2, "i"),
        ("t->ref", 6, "i"),
        ("Tarray.ref", 6, "i"),
        ("Tchannel.ref", 2, "i"),
        ("Tlist.ref", 2, "i"),
        ("Tptr.ref", 2, "i"),
        ("D2H(dv)->ref", 1, "u"),
        ("D2H(ds)->ref", 2, "u"),
        ("D2H(message)->ref", 1, "u"),
        ("D2H(t)->ref", 1, "u"),
    )
    for field, expected, kind in fields:
        pattern = r"(?<![\w&])" + re.escape(field) + r"(?=\s*(?:==|<=|>=|>|<))"
        source, count = re.subn(
            pattern, lambda match: "dvm_refcount_load_" + kind + "(&" + field + ")", source
        )
        if count != expected:
            raise ValueError("direct opcode observation drift: " + field)
    changes = (
        (
            "memmove(&references, (uchar*)type+offsetof(Type, ref), sizeof(references));",
            "references = dvm_refcount_load_i(&type->ref);",
            1,
        ),
        (
            "memmove(&references, (uchar*)D2H(DVM_R.M)+O(Heap, ref), sizeof(references));",
            observe("references", "D2H(DVM_R.M)"),
            1,
        ),
        (
            "memmove(&references, (uchar*)h+O(Heap, ref), sizeof(references));",
            observe("references", "h"),
            2,
        ),
        (
            "memmove(&references, (uchar*)buffer.array-sizeof(Heap)+offsetof(Heap, ref), sizeof(references));",
            observe("references", "(uchar*)buffer.array-sizeof(Heap)", "return 0;"),
            1,
        ),
        (
            "references++;\n\tmemmove((uchar*)h+O(Heap, ref), &references, sizeof(references));",
            "dvm_runtime_heap_retain(&h->ref);",
            1,
        ),
        (
            "references--;\n\t\tmemmove((uchar*)h+O(Heap, ref), &references, sizeof(references));",
            "(void)dvm_runtime_heap_release(&h->ref);",
            1,
        ),
    )
    source = replacements(source, changes)
    start = source.index("void\nxec(Prog *p)")
    dispatch = source[start:]
    if dispatch.count("p->R = DVM_R;") != 2:
        raise ValueError("dedicated dispatch root-publication boundary drift")
    dispatch = dispatch.replace("p->R = DVM_R;", "dvm_runtime_publish_dispatch_roots(p);")
    return '#include "dispatch_roots.h"\n#include "disvm/refcount.h"\n' + source[:start] + dispatch


# Final hosted Sys core: one whole-header snapshot, four byte-field reads,
# one debug observation, three Type retains and six descriptor preflights.
def adapt_runt(source):
    source = headers(source, 1)
    changes = [
        ("references = D2H(ptr)->ref;", "references = dvm_refcount_load_u(&D2H(ptr)->ref);", 1),
        ("Tptr.ref++;", "dvm_runtime_type_retain(&Tptr.ref);", 1),
        ("h->t->ref++;", "dvm_runtime_type_retain(&h->t->ref);", 1),
        ("Tbyte.ref++;", "dvm_runtime_type_retain(&Tbyte.ref);", 1),
    ]
    for field in ("Tptr.ref", "Tarray.ref", "Tbyte.ref"):
        changes += [
            (field + " <= 0", "dvm_refcount_load_i(&" + field + ") <= 0", 1),
            (field + " == 0x7fffffff", "dvm_refcount_load_i(&" + field + ") == 0x7fffffff", 1),
        ]
    for name, offset, count in (
        ("old", "O", 2),
        ("view", "offsetof", 1),
        ("result->previous", "offsetof", 1),
    ):
        before = (
            "memmove(&references, (uchar*)"
            + name
            + "-sizeof(Heap)+"
            + offset
            + "(Heap, ref), sizeof(references));"
        )
        changes.append((before, observe("references", "(uchar*)" + name + "-sizeof(Heap)"), count))
    return '#include "type_counts.h"\n#include "disvm/refcount.h"\n' + replacements(source, changes)
