"""Select guarded REAL arithmetic only after complete-body drift checks."""
import hashlib
import re

EXPECTED = {'negf': 'a72d42cb76d14c6933e37974887c2e6d234a8ab25512a26429050bfb20dc364e', 'addf': '8d2335928bfe176ad941fede3473b668ca411a8fbd9fcb58b36dbb3141e5ac69', 'subf': 'f96409cdb8cb6a947a988afc21923e34bda29578e285f3687dad6dac89f2826d', 'mulf': 'b43708a46e4ce10f6694c6fb7d88d2c12c44d0f3e667d4197a5b74f0df674c08', 'divf': 'b8ddd42771ce2880ea450f95446f0def4ad4124332c38de49af4d811fc7f4a32'}  # Complete retained native operation bodies.


def adapt(source):
    """Borrow source text; preserve default path and refuse missing/repeated/drifted bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded REAL arithmetic source drift: '+name)
        match = matches[0]
        body = match.group()
        anchor = '\t/* Reject malformed slots' if name == 'negf' else '\t/* Bound every used slot'
        operands = 'DVM_R.s, nil' if name == 'negf' else 'DVM_R.m, DVM_R.s'
        if body.count(anchor) != 1:
            raise ValueError('guarded REAL arithmetic insertion drift: '+name)
        code = '\tif(dvm_runtime_guarded_copies) {\n\t\tconst char *problem = dvm_runtime_real_arithmetic(I'+name.upper()+', '+operands+', DVM_R.d);\n\t\tif(problem != nil) {\n\t\t\terror((char *)problem);\n\t\t}\n\t\treturn;\n\t}\n'
        source = source[:match.start()]+body.replace(anchor,code+anchor)+source[match.end():]
    return '#include "shared_real_arithmetic.h"\n'+source
