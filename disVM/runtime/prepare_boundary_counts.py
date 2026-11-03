"""Convert reviewed Loader and exception retains in private hosted source views."""
import hashlib

EXPECTED = {  # Complete pre-adaptation native-register source views.
    'loader': '73c15fcd7da6699873eb54b7ad9549fef2bfc5f5d397909bd654a45422099038',
    'exception': 'b8ed18f22059962e99a7f74cf37f1afc96e4bb5260294a580d2b34c205b4a085',
}


def adapt(source, unit):
    """Return adapted owned text for unit, rejecting drift or repeated conversion.

    Counts require independently protected live objects and serialized admission;
    retain refusal is terminal. Existing marks, roots and stores keep their order.
    No source lifetime, payload permission or shared wire change is introduced.
    """
    if unit not in EXPECTED or hashlib.sha256(source.encode()).hexdigest() != EXPECTED[unit]:
        raise ValueError('hosted boundary-count source drift: '+unit)
    changes = {
        'loader': (('h->ref++;', 'dvm_runtime_heap_retain(&h->ref);'),
                   ('h->t->ref++;', 'dvm_runtime_type_retain(&h->t->ref);')),
        'exception': (('D2H(exstr)->ref++;', 'dvm_runtime_heap_retain(&D2H(exstr)->ref);'),
                      ('D2H(p->exval)->ref++;', 'dvm_runtime_heap_retain(&D2H(p->exval)->ref);')),
    }
    for before, after in changes[unit]:
        if source.count(before) != 1:
            raise ValueError('hosted retain boundary drift: '+before)
        source = source.replace(before, after)
    return '#include "heap_counts.h"\n#include "type_counts.h"\n'+source
