"""Select guarded LENA after exact retained-body drift checks."""
import hashlib
import re
from prepare_shared_bytes import guarded_publish

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
    source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_array_length.h"\n#include "shared_bytes.h"\n'+adapt_index(source)


def adapt_index(source):
    """Keep the shared address validator and guard the decoded scalar slots."""
    matches = list(re.finditer(r'static void\narrayindex\(int width\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != "4d4ca1ecfff1909002fbe383a4beae3357e7878c2d7ff0ec7ba03f8957ed5de6":
        raise ValueError('guarded array index source drift')
    match = matches[0]
    body = match.group()
    capture = '\tmemmove(&array, DVM_R.s, sizeof(array));\n\tmemmove(&index, DVM_R.d, sizeof(index));'
    body = body.replace(capture, '''    if(dvm_runtime_guarded_copies) {
        int replay;
        const char *problem = dvm_runtime_array_index_operands(DVM_R.s, DVM_R.d, &array, &index, &replay);
        if(replay) {
            return;
        }
        if(problem != nil) {
            error((char *)problem);
        }
    } else {
'''+capture+"""
    }""")
    publication = 'memmove(DVM_R.m, &address, sizeof(address));'
    body = body.replace(publication, guarded_publish('address', 'DVM_R.m', publication))
    return source[:match.start()]+body+source[match.end():]
