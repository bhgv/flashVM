"""Route reviewed scalar C opcodes and selected x64 emitters to one coordinator."""
import hashlib
import re

EXPECTED = {'consb': '95d21640ac91a7f0ecf39d28c7ac72dde154514c6890624797b4a04e5f1966c9',
 'consw': '2bf65cd504352b97d6d8cecea78dd5fcbc9c02f348344cd1e23139e10deb3958',
 'consl': 'ccc95e8a680d9d7b6cb52f50219f17432f20195f1e8c90bb4b066f7f7c8f44d6',
 'consf': '89ef237a2cfb72970545b3bb69d1ed957557610a8272f84432b8c2cd16014d22'}  # Exact adapted native C bodies.
EMITTER = '5d0f0014f1eaa5497ce900aafc2a2622bdfea736dcd5d26aab0382ea946ac1b6'  # Exact native scalar emitter before this adapter.


def adapt_xec(source):
    """Return selected C view; reject drift/double adaptation before publishing."""
    for name,digest in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}',source,re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != digest:
            raise ValueError('scalar constructor C drift: '+name)
        match = matches[0]
        body = match.group()
        width = {'consb':'sizeof(BYTE)','consw':'sizeof(WORD)','consl':'sizeof(LONG)','consf':'sizeof(REAL)'}[name]
        extent = 'IBY2WD' if name == 'consb' else width
        selection = '\n    if (dvm_runtime_scalar_cons(DVM_R.s, DVM_R.d, '+width+', '+extent+'))\n    {\n        return;\n    }'
        at = body.index('\n{')+2
        body = body[:at]+selection+body[at:]
        source = source[:match.start()]+body+source[match.end():]
    return '#include "scalar_cons.h"\n'+source


def adapt_emitter(source):
    """Selected SysV x64 tails inherit C return PC; other profiles stay original."""
    matches = list(re.finditer(r'static int\nx64scalarcons\(.*?\n\}',source,re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EMITTER:
        raise ValueError('scalar constructor emitter drift')
    match = matches[0]
    body = match.group()
    anchor = '\tunsigned long long entry;'
    selection = r"""
    if(dvm_runtime_guarded_copies) {
        if(!x64aggregateprofile(a) || !leafscalarconslayout(a, p->bytes) ||
           p->input != (p->bytes == 1 ? IBY2WD : p->bytes)) {
            return 0;
        }
        if(p->bytes == 1) {
            entry = (unsigned long long)dvm_runtime_scalar_1_fast;
        }else if(p->bytes == 4) {
            entry = (unsigned long long)dvm_runtime_scalar_4_fast;
        }else if(p->bytes == 8) {
            entry = (unsigned long long)dvm_runtime_scalar_8_fast;
        }else{
            return 0;
        }
        x64byte(b, 0x48);
        x64byte(b, 0xb8);
        x64word(b, entry);
        x64word(b, entry>>32);
        /* Tail JMP leaves no return address inside the retireable RX mapping. */
        x64byte(b, 0xff);
        x64byte(b, 0xe0);
        return 1;
    }
"""
    if body.count(anchor) != 1:
        raise ValueError('scalar constructor emitter insertion drift')
    body = body.replace(anchor,anchor+'\n'+selection,1)
    return '#include "scalar_cons.h"\n#include "shared_bytes.h"\n'+source[:match.start()]+body+source[match.end():]
