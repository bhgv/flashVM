# Copyright (c) 2026 bhgv
# [GC:] EN: Adapt reviewed collector metadata in private generated sources.
# RU: Адаптировать проверенные метаданные GC в приватных генерируемых исходниках.
"""Adapt only reviewed hosted C collector/allocation scalar metadata sites."""

import hashlib

HEADER = "fecb9548ce6de459791ccab2ca8b175fae8d2dc0ba16a8c3f7dfd3e73db84f49"  # Reviewed native Heap/color/propagator declarations.


# [GC:] EN: Validate borrowed header text against the reviewed upstream version.
# Heap/Type/color layouts are unchanged by c7c60337's Module-only extensions.
# Return nothing; reject drift before any generated collector mutation.
# RU: Проверить заимствованный header по проверенной upstream-версии. Расширения
# Module в c7c60337 не меняют Heap/Type/color. При дрейфе отказать до адаптации GC.
def validate_header(source):
    """Reject changed native metadata layout/color constants before adapting C."""
    if hashlib.sha256(source.encode()).hexdigest() != HEADER:
        raise ValueError("hosted GC scalar native header drift")


EXPECTED = {  # Complete final hosted views before this adaptation.
    "gc": "32e2aa67756c05f37852370c77489993083e17c4680d89bbe3f3b4d0eca571a0",
    "heap": "7557521f382cb0562334f72dbed945f2d74b79e6f86038334a0e1d22cfcdefbe",
}


# [GC:] EN: Borrow source/unit, return owned adapted text or reject drift.
# Only atomic scalar metadata changes; root/payload lifetime stays unchanged.
# RU: Заимствовать source/unit, вернуть адаптацию либо отказать при дрейфе.
# Меняются лишь атомарные скалярные метаданные, не жизнь корней/payload.
def adapt(source, unit):
    """Return owned adapted C text or reject source drift/repeated application.

    Stores/loads are scalar metadata only. Collector phase/epoch, graph lifetime,
    payload/root barriers and generated JIT memory accesses remain separate.
    """
    if unit not in EXPECTED or hashlib.sha256(source.encode()).hexdigest() != EXPECTED[unit]:
        raise ValueError("hosted GC scalar source drift: " + unit)
    changes = [
        (
            "h->color = mutator;",
            "dvm_runtime_gc_store(&h->color, dvm_runtime_gc_load(&mutator));",
            2 if unit == "gc" else 3,
        )
    ]
    if unit == "gc":
        changes += [
            ("h->color == propagator", "dvm_runtime_gc_load(&h->color) == propagator", 2),
            ("h->color == sweeper", "dvm_runtime_gc_load(&h->color) == sweeper", 1),
            ("D2H(l)->color = propagator;", "dvm_runtime_gc_store(&D2H(l)->color, propagator);", 1),
            ("(ulong)h->t, h->color);", "(ulong)h->t, dvm_runtime_gc_load(&h->color));", 1),
            ("mutator = gccolor % 3;", "dvm_runtime_gc_store(&mutator, gccolor % 3);", 1),
            ("nprop = 1;", "dvm_runtime_gc_store(&nprop, 1);", 1),
            ("if(nprop == 0)", "if(dvm_runtime_gc_exchange(&nprop, 0) == 0)", 1),
            ("nprop = 0;", "/* Pending work was consumed by the preceding atomic exchange. */", 1),
        ]
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError("hosted GC scalar boundary drift: " + before)
        source = source.replace(before, after)
    return '#include "gc_fields.h"\n' + source
