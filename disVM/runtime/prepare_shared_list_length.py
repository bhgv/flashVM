"""Select guarded LENL only after a full retained-body drift check."""
import hashlib
import re

EXPECTED = '6c964dd6e71ababe562debe9752fd67bd02dd19534c8c0072dfb36fb2d365248'  # Existing native LENL, no new file opcode.


def adapt(source):
    """Return startup-selected source; refuse changed/missing/repeated body."""
    matches = list(re.finditer(r'OP\(lenl\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded list-length source drift')
    body = matches[0].group()
    anchor = '\t/* Neither the scan'
    if body.count(anchor) != 1:
        raise ValueError('guarded list-length insertion drift')
    guarded = '''    if(dvm_runtime_guarded_copies) {
        const char *problem = dvm_runtime_list_length(DVM_R.s, DVM_R.d);
        if(problem != nil) {
            error((char *)problem);
        }
        return;
    }
'''
    body = body.replace(anchor, guarded+anchor)
    match = matches[0]
    return '#include "shared_list_length.h"\n'+source[:match.start()]+body+source[match.end():]
