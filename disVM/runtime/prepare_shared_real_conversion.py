"""Select guarded FP conversion only after complete-body drift checks."""
import hashlib
import re

EXPECTED = {'cvtrf': 'd9cdee7af5158f9c810e29a45c593fd1ee6223730c073314f6e5e625085972dd', 'cvtfr': '662621abbff8ccba22403769aead2e08a066b7542796ee35f23b3240d2278fae', 'cvtwf': '849551ecd34613e896cb0a69ae09d83e33c43054871217b6d8af91add1812e06', 'cvtlf': 'ee9d62f56cdc01c766db37d3651cab8c0e8754517db90a48862244af25f4c926', 'cvtfw': 'df7aab3bb0d105fb65a80a8d2894e205e96c87c583e68d0ae00b4bb64dbf415f', 'cvtfl': '76dee3dfb7856e78bd6b4bcabd46cb39879fbfb7e8d6bd76b877740b9ab44b04'}  # Complete retained native conversion bodies.


def adapt(source):
    """Borrow source text; preserve default path and refuse missing/repeated/drifted bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded FP conversion source drift: '+name)
        match = matches[0]
        body = match.group()
        anchor = '\t/* Reject malformed slots'
        if body.count(anchor) != 1:
            raise ValueError('guarded FP conversion insertion drift: '+name)
        code = '\tif(dvm_runtime_guarded_copies) {\n\t\tconst char *problem = dvm_runtime_real_conversion(I'+name.upper()+', DVM_R.s, DVM_R.d);\n\t\tif(problem != nil) {\n\t\t\terror((char *)problem);\n\t\t}\n\t\treturn;\n\t}\n'
        source = source[:match.start()]+body.replace(anchor,code+anchor)+source[match.end():]
    return '#include "shared_real_conversion.h"\n'+source
