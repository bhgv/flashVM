"""Insert three-span arithmetic only after complete final-body drift checks."""
import hashlib
import re

EXPECTED = {
    'addb': 'c30d33dbc5b8a0b128b53e5459afb09b91bb753af5fa0d8398b7cbcafb17bcd1',
    'addw': '0074967980770184662f9b78f8710b40c9a205e2bd3061e72c87c3ae43c7a66b',
    'addl': 'f3c48e610071b335e20ce0cadfba1973d2254e7d7fcfd4162a71610cb780495d',
    'subb': 'd1ef2c5709fac6587dc15b8fc0f061d250889d03037fcb9114e2ca3256e1064e',
    'subw': '8c3b4a3343411023e5ff5cdbbbef01f470312abcf6df25ae0cb91be071cc7801',
    'subl': '2790cacb4d1f8191e385ebcdef7bf3aa63cff38f33a72441f4ed7da535c5309b',
    'andb': 'b26de914e6d9b6bcf2e6acd3fcfa77806db089de4af64e7459c63c298f8a3abe',
    'andw': '852704eb5558040b2d96c735ff3f559d9ba1677ac16dc01c497bda7d6592c125',
    'andl': 'ed1228c2b40c59df3cca67b6b04a015e2e4afd9afd791c09681ac1218c941e04',
    'orb': '808ff902ea52d30ef3413e4720bca748a096c7fbc4a59f8faa3e1779d0bf4985',
    'orw': 'a833cceb6a209ca8e27a2a0651efce0ec5d749bed9e9c06e859543f8a0aa529e',
    'orl': '94b407942dc54f4ab7be707cf2d371b5d2be286db93bf40cbdd6194de60b919c',
    'xorb': 'a1b915735caa3e400eed49f04488b3e4444a8e5057df57420d7142c20d9b9312',
    'xorw': '4af11528e6273e775b7494aab697f21b945206d828da7cfbc7e87c23b77bf166',
    'xorl': '28fd84528be5e96ef5a289e7573f48d74677b323d2900c7039c36c5587d85b16',
    'mulb': 'a8bc250c963cf04875952b9379be33dd4d7410368c20981059225e063ecde7a6',
    'mulw': '268b7e1999b0b6ce39711f052bba99765fb4a0e27dd687fb8f378707d61155eb',
    'mull': '2af6384807fe5e76216f66c532400c6f558049ffb4ed30578978646bb84ad4f0',
    'divb': 'eea3cc4fe38145e740442ee788f196405434646b9b2be41f54dec339854c1859',
    'divw': '411257bd6cbc6052534969cc6df82ed977cb537a9cc820f9808c2b0d60fe6c51',
    'divl': '96007332eca2e205487eb9ec03b4183888fa8f89003ff1f10b406ded76a8c7d2',
    'modb': '936305396705ce473e5f305a59b37ec324c78a92122f8b30b5fe22144eba2e62',
    'modw': '4fc6e53eb3c79df1e7326715980ab70fb55fcb7fa8e6a21337238d407cc1b5f1',
    'modl': 'a1b0b97be03256978acd538b7d5905a71a348d366ddbaa3e47796e3ee1933d1e',
}  # Retained 24 full bodies; no file opcode changes.


def adapt(source):
    """Return startup-selected code; refuse missing/changed/repeated bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded arithmetic source drift: '+name)
        body = matches[0].group()
        anchor = '\t/* A zero divisor wins' if name.startswith(('div', 'mod')) else '\t/* Bound all decoded spans'
        if body.count(anchor) != 1:
            raise ValueError('guarded arithmetic insertion drift: '+name)
        body = body.replace(anchor, '\tif(dvm_runtime_guarded_copies) {\n\t\tconst char *problem = dvm_runtime_arithmetic(I'+name.upper()+', DVM_R.m, DVM_R.s, DVM_R.d);\n\t\tif(problem != nil) {\n\t\t\terror((char *)problem);\n\t\t}\n\t\treturn;\n\t}\n'+anchor)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_arithmetic.h"\n'+source
