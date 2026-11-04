"""Select guarded native real branches after exact complete-body drift checks."""
import hashlib
import re

EXPECTED = '84e4c9a72f05b68ab75ea59b094982bea660c60e90528734866f99d91a6fb22e'  # Existing complete native helper; no new opcode.


def adapt(source):
    """Borrow source text; return adapted text or raise on missing/repeated/drifted helper."""
    matches = list(re.finditer(r'static void\nbranchreal\(int condition\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded real-branch source drift')
    match = matches[0]
    body = match.group()
    anchor = '    /* Bound both source slots'
    if body.count(anchor) != 1:
        raise ValueError('guarded real-branch insertion drift')
    guarded = '''    if(dvm_runtime_guarded_copies) {
        const char *problem = dvm_runtime_real_branch(condition, DVM_R.s, DVM_R.m, DVM_R.d, &DVM_R.PC);
        if(problem != nil) {
            error((char *)problem);
        }
        return;
    }
'''
    body = body.replace(anchor, guarded+anchor)
    return '#include "shared_real_branch.h"\n'+source[:match.start()]+body+source[match.end():]
