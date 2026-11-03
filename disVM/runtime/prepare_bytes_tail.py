"""Opt-in hosted x64 scalar/raw copies tail-enter their guarded C opcodes."""
import hashlib
import re

EXPECTED = "332aec031d78dd5296ba2385b36543c2fdece63fbfd6ef6b902820788b6886d0"  # Original complete leafemit body; no shared ABI field/ID added.


def adapt(source):
    """Bind actual native layout and reviewed emitter before inserting sixteen tails.

    Other opcodes/architectures and the default profile retain old emission.
    Startup selection precedes code cache publication; no dynamic mode switching.
    """
    matches = list(re.finditer(r'static int\nleafemit\(.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('x64 guarded copy emitter drift')
    helper = '''#include "bytes_fast.h"
#include "shared_bytes.h"


/* Checked C baseline only; original return PC survives errors/cache retirement. */
static int
x64guardedcopy(u32int *out, int cap, LeafLayout *abi, int op)
{
    X64Buf b;
    unsigned long long entry = (unsigned long long)dvm_runtime_bytes_fast;
    if(abi->wordbytes != sizeof(WORD) || sizeof(BYTE) != 1 || sizeof(LONG) != 8 || sizeof(REAL) != 8 ||
       abi->s != O(REG, s) || abi->d != O(REG, d) || abi->m != O(REG, m) || abi->module != O(REG, M)) {
        return 0;
    }
    b.n = 0;
    x64byte(&b, 0xf3);
    x64byte(&b, 0x0f);
    x64byte(&b, 0x1e);
    x64byte(&b, 0xfa);
    x64byte(&b, 0xbe);
    x64word(&b, op);
    x64byte(&b, 0x48);
    x64byte(&b, 0xb8);
    x64word(&b, entry);
    x64word(&b, entry>>32);
    x64byte(&b, 0xff);
    x64byte(&b, 0xe0);
    while(b.n%4) {
        x64byte(&b, 0x90);
    }
    return leafcodecopy(out, cap, b.bytes, b.n/4);
}


'''
    body = matches[0].group()
    anchor = '\tb.n = 0;\n\t/* ENDBR64'
    if body.count(anchor) != 1:
        raise ValueError('x64 guarded copy entry anchor drift')
    body = body.replace(anchor, '''    if(dvm_runtime_guarded_copies &&
       (op == IMOVB || op == IMOVW || op == IMOVL || op == IMOVF || op == IMOVM ||
        op == IHEADB || op == IHEADW || op == IHEADL || op == IHEADF || op == IHEADM ||
        op == ICVTBW || op == ICVTWB || op == ICVTWS || op == ICVTSW ||
        op == ICVTWL || op == ICVTLW)) {
        return x64guardedcopy(out, cap, abi, op);
    }
'''+anchor, 1)
    return source[:matches[0].start()]+helper+body+source[matches[0].end():]
