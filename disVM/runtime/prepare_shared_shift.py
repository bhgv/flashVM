"""Select guarded shifts after complete retained-body drift checks."""
import hashlib
import re

EXPECTED = {
    'shlb': 'f224196fe51f54c277fb792cb992aae8a16c74f5fd4316a1141013c5cdaec4db',
    'shlw': '6af6effc0427e920479d841b1fe4f55d700bd61104115a501edef5d2b4e7e607',
    'shll': '9d24680fba44cd4296d28e9a0d78a609914a6afcfbb4bfcbe747a27293c471c4',
    'shrb': '69279a01c05d5bf034631d4f54726b6b52f02980a4e8f24f8c54f2240dafc2fa',
    'shrw': '78365253681fdea9c05eb68221c9cf809dff9bd65918b2bc5e63664f8f604021',
    'shrl': '95bf4e86e800b1d42c17293e76fd08456172c184255e895d39a65f8ca047b7dd',
    'lsrw': 'b5b441c3fcf689925de9a34dc23961d8e2e50fd54cfbcb760ff9be1414ce7383',
    'lsrl': '820834f7408cfa4648a64a573d79784d42bcdc80580ac0839396d96fd05f773f',
}  # Existing full native bodies, no new file opcodes.


def adapt(source):
    """Return startup-selected source; refuse changed/repeated/missing bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded shift source drift: '+name)
        body = matches[0].group()
        anchor = '    /* Reject malformed count slots'
        if body.count(anchor) != 1:
            raise ValueError('guarded shift insertion drift: '+name)
        body = body.replace(anchor, '    if(dvm_runtime_guarded_copies) {\n        const char *problem = dvm_runtime_shift(I'+name.upper()+', DVM_R.m, DVM_R.s, DVM_R.d);\n        if(problem != nil) {\n            error((char *)problem);\n        }\n        return;\n    }\n'+anchor)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "shared_shift.h"\n'+source
