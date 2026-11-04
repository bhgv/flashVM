"""Select raw CONSM coordinator after exact native C body drift check."""
import hashlib
import re

EXPECTED = '5b7428a1c25f6e5e6e2150091e6a4c64b4d65e6c6a95a1bfb2979622ff301748'  # Existing adapted native CONSM body.


def adapt(source):
    """Return selected hosted body; reject drift/repeated adaptation before write."""
    matches = list(re.finditer(r'OP\(consm\)\n\{.*?\n\}',source,re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('raw CONSM source drift')
    match = matches[0]
    body = match.group()
    at = body.index('\n{')+2
    selection = '\n    if (dvm_runtime_raw_cons(DVM_R.s, DVM_R.d, DVM_R.m))\n    {\n        return;\n    }'
    body = body[:at]+selection+body[at:]
    return '#include "raw_cons.h"\n'+source[:match.start()]+body+source[match.end():]
