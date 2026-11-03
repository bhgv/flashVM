"""Select guarded scalar/raw list heads with exact retained-body drift checks."""
import hashlib
import re

EXPECTED = {
    'headb': 'ce3d56d4c9e3e5c7c1aaee2aa052c949e886ffbce1cdf78c481466301b0106b7',
    'headw': '9a9013e450873dcf7a45c365cd826eab64129edae213f2ab30b11248688a7fe5',
    'headl': 'b258b0813045dba0db820ac8f2c1efe9f3abc0f11bb196be2ab686315fdee342',
    'headf': 'c237e0057ab61b660d2658f336b2b493022b4cebc817834838ef188f59bafb5c',
    'headm': 'f0e11b47a19e60858db5c15ae52514bd970e69eeb9710ea82b5360c65b61b700',
}  # Complete final native bodies; no new file opcodes.


def adapt(source):
    """Preserve default bodies, reject drift before startup-profile insertion."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded scalar-list source drift: '+name)
        body = matches[0].group()
        anchor = '\tl = listhead();' if name == 'headm' else '\t/* Validate the result'
        if body.count(anchor) != 1:
            raise ValueError('guarded scalar-list insertion drift: '+name)
        body = body.replace(anchor, '\tif(dvm_runtime_guarded_copies) {\n\t\tdvm_runtime_list_bytes_copy(I'+name.upper()+');\n\t\treturn;\n\t}\n'+anchor)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_list_bytes.h"\n'+source
