"""Select rooted HEADP/TAIL only for the explicit guarded-copy profile."""
import hashlib
import re

EXPECTED = {
    'headp': 'de94deaa22a45c0e561c72d7192a9ab8e7b1782453c703935e27e77a21083d4c',
    'tail': 'ae4ba166ef3c0aa2b302522513a30a7711b04f8536e8a7797be02b4cb9627de2',
}  # Complete reviewed final native opcode bodies.


def adapt(source):
    """Reject body drift before changing owned text; leave default bodies intact."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded pointer-list source drift: '+name)
        body = matches[0].group()
        anchor = '\tList *l;\n'
        if body.count(anchor) != 1:
            raise ValueError('guarded pointer-list declaration drift: '+name)
        body = body.replace(anchor, anchor+'\n\tif(dvm_runtime_guarded_copies) {\n\t\tdvm_runtime_list_pointer_copy(DVM_R.s, DVM_R.d, '+str(int(name == 'tail'))+');\n\t\treturn;\n\t}\n')
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_list_pointer.h"\n'+source
