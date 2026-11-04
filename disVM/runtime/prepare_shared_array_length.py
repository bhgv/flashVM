"""Select guarded LENA after exact retained-body drift checks."""
import hashlib
import re

EXPECTED = '5b53882224e3813188e886c444068e0460b4ea66020912810b411b06b2324615'  # Complete existing native LENA body, not a new opcode.


def adapt(source):
    """Return startup-selected LENA; refuse changed/missing/repeated bodies."""
    matches = list(re.finditer(r'OP\(lena\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded array-length source drift')
    body = matches[0].group()
    anchor = '\t/* Length queries'
    if body.count(anchor) != 1:
        raise ValueError('guarded array-length insertion drift')
    guarded = '''    if(dvm_runtime_guarded_copies) {
        const char *problem = dvm_runtime_array_length(DVM_R.s, DVM_R.d);
        if(problem != nil) {
            error((char *)problem);
        }
        return;
    }
'''
    body = body.replace(anchor, guarded+anchor)
    match = matches[0]
    return '#include "shared_array_length.h"\n'+source[:match.start()]+body+source[match.end():]
