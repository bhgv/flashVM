# Copyright (c) 2026 bhgv
# EN: Convert reviewed retain sites in hosted Loader/exception source views.
# RU: Преобразовать проверенные retain-границы Loader/exception в host-исходниках.
"""Convert reviewed Loader and exception retains in private hosted source views."""

import hashlib

EXPECTED = {  # Complete pre-adaptation native-register source views.
    "loader": "d578a15dfc10665ca72135533cf3312a7d782a81e485e75ee4a77541c3580cdb",
    "exception": "b8ed18f22059962e99a7f74cf37f1afc96e4bb5260294a580d2b34c205b4a085",
}


# EN: Borrow source/unit and return private text after exact version/site checks.
# Loader c7c60337 adds checked PCs and transactional startup; its two retain sites
# preserve ownership order and use the existing atomic host counters.
# RU: Заимствовать source/unit и вернуть текст после точных проверок версии/границ.
# Loader c7c60337 добавляет checked PC и транзакционный старт; два retain сохраняют
# порядок владения и используют существующие атомарные host-счётчики.
def adapt(source, unit):
    """Return adapted owned text for unit, rejecting drift or repeated conversion.

    Counts require independently protected live objects and serialized admission;
    retain refusal is terminal. Existing marks, roots and stores keep their order.
    No source lifetime, payload permission or shared wire change is introduced.
    """
    if unit not in EXPECTED or hashlib.sha256(source.encode()).hexdigest() != EXPECTED[unit]:
        raise ValueError("hosted boundary-count source drift: " + unit)
    changes = {
        "loader": (
            ("h->ref++;", "dvm_runtime_heap_retain(&h->ref);"),
            ("h->t->ref++;", "dvm_runtime_type_retain(&h->t->ref);"),
        ),
        "exception": (
            ("D2H(exstr)->ref++;", "dvm_runtime_heap_retain(&D2H(exstr)->ref);"),
            ("D2H(p->exval)->ref++;", "dvm_runtime_heap_retain(&D2H(p->exval)->ref);"),
        ),
    }
    for before, after in changes[unit]:
        if source.count(before) != 1:
            raise ValueError("hosted retain boundary drift: " + before)
        source = source.replace(before, after)
    return '#include "heap_counts.h"\n#include "type_counts.h"\n' + source
