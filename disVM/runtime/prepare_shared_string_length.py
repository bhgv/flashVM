"""Select guarded LENC after exact retained-body drift checks."""
import hashlib
import re

EXPECTED = 'b478bfe0e38ecb12c4f038d1a4defe39cf834b0b1040d2f0b2457e0fcec6b992'  # Complete existing native LENC body, not a new opcode.


def adapt(source):
    """Return startup-selected LENC; refuse changed/missing/repeated bodies."""
    matches = list(re.finditer(r'OP\(lenc\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded string-length source drift')
    body = matches[0].group()
    anchor = '\t/* The output must'
    if body.count(anchor) != 1:
        raise ValueError('guarded string-length insertion drift')
    guarded = '''    if(dvm_runtime_guarded_copies) {
        const char *problem = dvm_runtime_string_length(DVM_R.s, DVM_R.d);
        if(problem != nil) {
            error((char *)problem);
        }
        return;
    }
'''
    body = body.replace(anchor, guarded+anchor)
    match = matches[0]
    return '#include "shared_string_length.h"\nextern int dvm_runtime_guarded_copies; /* Startup-selected native guard profile. */\n'+source[:match.start()]+body+source[match.end():]
