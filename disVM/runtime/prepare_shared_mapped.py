"""Route reviewed mapped moves through the opt-in ownership transaction."""
import hashlib
import re
from prepare_shared_bytes import guarded_snapshot

BODIES = (
    (r'OP\(movmp\)\n\{.*?\n\}',
     '2bb3a55414025b3f5d79e81d6b3eb06c57fe4beddc233051986b63748f4ce2c6'),
    (r'void\nmovtmp\(void\)\n\{.*?\n\}',
     'ec545b79a9105cf2bac1640ede7e25b41d7c0e6fbb00fc9112e3da2b799b9b2d'),
)


def adapt(source):
    """Preserve validation and default dispatch, rejecting source drift."""
    anchor = '\tsource = DVM_R.s;\n\tdestination = DVM_R.d;'
    guarded = (
        '\n\tif(dvm_runtime_guarded_copies) {'
        '\n\t\tdvm_runtime_mapped_copy(source, destination, t, size);'
        '\n\t\treturn;\n\t}'
    )
    for pattern, digest in BODIES:
        matches = list(re.finditer(pattern, source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != digest:
            raise ValueError('hosted mapped-copy body drift: ' + pattern)
        match = matches[0]
        body = match.group()
        if body.count(anchor) != 1:
            raise ValueError('hosted mapped-copy publication boundary drift')
        variable = 'index' if pattern.startswith(r'OP') else 't'
        capture = f'memmove(&{variable}, DVM_R.m, sizeof({variable}));'
        if body.count(capture) != 1:
            raise ValueError('hosted mapped-copy metadata capture drift')
        body = body.replace(capture, guarded_snapshot(variable, capture), 1)
        source = source[:match.start()] + body.replace(anchor, anchor + guarded, 1) + source[match.end():]
    return '#include "shared_mapped.h"\n#include "shared_bytes.h"\n' + source
