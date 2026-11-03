"""Adapt only reviewed hosted C collector/allocation scalar metadata sites."""
import hashlib

HEADER = 'd7edfa14ffd98be92acf4904568f0d3807418bf769e282b9de4d2daf617f317c'  # Reviewed native Heap/color/propagator declarations.


def validate_header(source):
    """Reject changed native metadata layout/color constants before adapting C."""
    if hashlib.sha256(source.encode()).hexdigest() != HEADER:
        raise ValueError('hosted GC scalar native header drift')


EXPECTED = {  # Complete final hosted views before this adaptation.
    'gc': '32e2aa67756c05f37852370c77489993083e17c4680d89bbe3f3b4d0eca571a0',
    'heap': '7557521f382cb0562334f72dbed945f2d74b79e6f86038334a0e1d22cfcdefbe',
}


def adapt(source, unit):
    """Return owned adapted C text or reject source drift/repeated application.

    Stores/loads are scalar metadata only. Collector phase/epoch, graph lifetime,
    payload/root barriers and generated JIT memory accesses remain separate.
    """
    if unit not in EXPECTED or hashlib.sha256(source.encode()).hexdigest() != EXPECTED[unit]:
        raise ValueError('hosted GC scalar source drift: '+unit)
    changes = [('h->color = mutator;',
                'dvm_runtime_gc_store(&h->color, dvm_runtime_gc_load(&mutator));', 2 if unit == 'gc' else 3)]
    if unit == 'gc':
        changes += [
            ('h->color == propagator', 'dvm_runtime_gc_load(&h->color) == propagator', 2),
            ('h->color == sweeper', 'dvm_runtime_gc_load(&h->color) == sweeper', 1),
            ('D2H(l)->color = propagator;', 'dvm_runtime_gc_store(&D2H(l)->color, propagator);', 1),
            ('(ulong)h->t, h->color);', '(ulong)h->t, dvm_runtime_gc_load(&h->color));', 1),
            ('mutator = gccolor % 3;', 'dvm_runtime_gc_store(&mutator, gccolor % 3);', 1),
            ('nprop = 1;', 'dvm_runtime_gc_store(&nprop, 1);', 1),
            ('if(nprop == 0)', 'if(dvm_runtime_gc_load(&nprop) == 0)', 1),
            ('nprop = 0;', 'dvm_runtime_gc_store(&nprop, 0);', 1),
        ]
    for before, after, count in changes:
        if source.count(before) != count:
            raise ValueError('hosted GC scalar boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "gc_fields.h"\n'+source
