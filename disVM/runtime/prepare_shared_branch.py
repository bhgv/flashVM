"""Select guarded integer branches after exact retained-body drift checks."""
import hashlib
import re

EXPECTED = {'beqb': '3d22f7365d98218d9101bf6c78869cf1272c369121371a83e210c9dadc97394d',
 'bneb': '06c4b2796143f7c549d72938ff0fe1c1f35ac2fec1189961bee083f0c025a248',
 'bltb': '46cb92548aab64863a01dc8dc11ab474adde01b830808a3c88e5f32f66f67502',
 'bleb': 'fda04ca2c0ae855022cd13195bd7a576d325ec12abd78a47998e5e11000916b6',
 'bgtb': 'd56713dc6efd5af6f101b01b544a2a2736c1a569053d8ebe7eddd5587617a70c',
 'bgeb': 'c9d4b04edaf4cfcd0e05702e772f4a45fb2d92c208ffd199c5582c9c54b61074',
 'beqw': 'aca833eeaafa01893aa21cf8246818af0d1aa43ef0e4788b808cb91f670e10a1',
 'bnew': '960ce43d313726e220aa2df8b0ebbe2e27d14da1690e019a9787376ad15cd5a6',
 'bltw': 'dd8f5f5841de9e16db47b691cde9ddee2e2ace4ab45f1c28774655876bebb2c5',
 'blew': '72f01f3747ef5dbc0d5bfe6e59dd35fb74b98e2570f3d190c28fdf707add70d1',
 'bgtw': '720c226a476d0b3c8dfc382e871b90307182b391bd7d620b6288317139d926b9',
 'bgew': '1152ea4ed58b00ad391875918d0c06c481f0ebd729975999d1fbeeed77d5d7c0',
 'beql': '3bc843765e19c31ada45d98bcd669e1acef2698d1af2d978667e7d7a52cd817c',
 'bnel': '41f2b78cdb0e4e47f36b56cdc6657efef429db9c7e353fee8eca0e8e7db4398e',
 'bltl': '927002ce764b1e915d4759919248fc256ab713b6c5bc248eb05003c4b0b8ed97',
 'blel': 'd8fa1567199b83c88f62db46be46b5636168d0b2a8d615a78096bcfb10b4de57',
 'bgtl': '6905b4473425cf3a3c2d11d3f4ae79ec309241df0d0b98ac67b9f6fbb793da6c',
 'bgel': 'e74633ccdbb3a05c136ba9ff8c378d76669d2b1eb588e880910233414db911f1'}  # Existing complete native bodies; no wire IDs added.


def adapt(source):
    """Return startup-selected source or refuse missing/changed/repeated bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded branch source drift: '+name)
        body = matches[0].group()
        anchor = '    if(DVM_R.s == nil'
        if body.count(anchor) != 1:
            raise ValueError('guarded branch insertion drift: '+name)
        inserted = '    if(dvm_runtime_guarded_copies) {\n        const char *problem = dvm_runtime_integer_branch(I'+name.upper()+', DVM_R.s, DVM_R.m, DVM_R.d, &DVM_R.PC);\n        if(problem != nil) {\n            error((char *)problem);\n        }\n        return;\n    }\n'
        body = body.replace(anchor, inserted+anchor)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_branch.h"\n'+source
