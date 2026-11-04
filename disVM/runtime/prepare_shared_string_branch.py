"""Select guarded native string branches after exact complete-body drift checks."""
import hashlib
import re

EXPECTED = 'ee6b24ff1f89bfb7194f02298efd2ff0763cec547420d886262f8d144661514c'  # Existing complete native helper; no new opcode.


def adapt(source):
    """Borrow source text; return adapted text or raise on missing/repeated/drifted helper."""
    matches = list(re.finditer(r'static void\nbranchstring\(int condition\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded string-branch source drift')
    match = matches[0]
    body = match.group()
    anchor = '\t/* Both decoded owner slots'
    if body.count(anchor) != 1:
        raise ValueError('guarded string-branch insertion drift')
    guarded = '''    if(dvm_runtime_guarded_copies) {
        const char *problem = dvm_runtime_string_branch(condition, DVM_R.s, DVM_R.m, DVM_R.d, &DVM_R.PC);
        if(problem != nil) {
            error((char *)problem);
        }
        return;
    }
'''
    body = body.replace(anchor, guarded+anchor)
    return '#include "shared_string_branch.h"\n'+source[:match.start()]+body+source[match.end():]
