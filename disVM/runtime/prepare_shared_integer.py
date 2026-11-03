"""Select guarded integer conversion bodies only after exact drift checks."""
import hashlib
import re

EXPECTED = {'cvtbw': '6dbf45ed50ff3d4f7550b2150f4f5ba8d2508cb9b9fd1235ce7f43a7622449e0', 'cvtwb': 'ee90b9ad6a964851c0d48f8afd207293b3eac9a55b06e3b5dd87dae19f073848', 'cvtws': 'd6a5f8a5471dddc650db9a7112f0811cf81265426c88dcb3a33e45c870cdd043', 'cvtsw': 'efbde0a43b7e47a4d571c613fd3315f15da1f2d138dd249a06679a9e7e939f95', 'cvtwl': '826b426c9142a1b14ef99f85794a27e24333f86a26def3ef6cdc2c754109fe0c', 'cvtlw': 'ac1583a496a2ec4e6f12bfc1bd04ddafb95e5a7209dfec27b28ef4865dc80e2c'}  # Complete final retained operation bodies.


def adapt(source):
    """Preserve default semantics and reject changed/repeated operation bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded integer source drift: '+name)
        body = matches[0].group()
        anchor = '\t/* Reject malformed slots'
        if body.count(anchor) != 1:
            raise ValueError('guarded integer insertion drift: '+name)
        body = body.replace(anchor, '\tif(dvm_runtime_guarded_copies) {\n\t\tconst char *problem = dvm_runtime_integer_convert(I'+name.upper()+', DVM_R.s, DVM_R.d);\n\t\tif(problem != nil) {\n\t\t\terror((char *)problem);\n\t\t}\n\t\treturn;\n\t}\n'+anchor)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_integer.h"\n'+source
