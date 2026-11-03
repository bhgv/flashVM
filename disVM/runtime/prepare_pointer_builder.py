"""Route the reviewed hosted pointer-cons opcode through its C transaction."""
import hashlib
import re

EXPECTED = "505f6ecd507e1f6e4d4fe8dfa14437f2110d8c24e42780a2d54e443f53a84913"  # Reviewed hosted opcode body.


# source is fully adapted private executor text. Reject source drift/double
# adaptation before publishing a new view; preserve all unrelated operations.
def adapt(source):
    matches = list(re.finditer(r'OP\(consp\)\n\{.*?\n\}', source, re.DOTALL))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('hosted pointer-cons changed; review transaction boundary')
    match = matches[0]
    body = 'OP(consp)\n{\n    dvm_runtime_pointer_cons(DVM_R.s, DVM_R.d);\n}'
    return '#include "pointer_builder.h"\n'+source[:match.start()]+body+source[match.end():]
