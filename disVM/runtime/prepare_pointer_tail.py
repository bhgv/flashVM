"""Replace hosted x64 pointer-cons machine RMWs with a shared C tail entry."""
import hashlib
import re

EXPECTED = '7bfa57558ff9ea2df7ca2bc9e16baf2ad985e68dc658ddf317441d47f97d6fdf'  # Reviewed pinned x64 pointer-cons emitter body.


# source is private hosted emitter text. Reject drift before replacing only
# pointer cons. REG remains in SysV rdi; tail JMP creates no module return PC.
# Other backends retain the existing decline until their C ABI is exercised.
def adapt(source):
    matches = list(re.finditer(r'static int\nx64pointercons\(X64Buf \*b, LeafLayout \*a\)\n\{.*?\n\}', source, re.DOTALL))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('x64 pointer-cons emitter changed; review C tail ABI')
    match = matches[0]
    body = '''static int
x64pointercons(X64Buf *b, LeafLayout *a)
{
    unsigned long long entry;
    if(!leafscalarconslayout(a, 8)) {
        return 0;
    }
    entry = (unsigned long long)dvm_runtime_pointer_fast;
    x64byte(b, 0x48);
    x64byte(b, 0xb8);
    x64word(b, entry);
    x64word(b, entry>>32);
    /* REG stays in rdi; original C caller's return address stays on stack. */
    x64byte(b, 0xff);
    x64byte(b, 0xe0);
    return 1;
}'''
    return '#include "pointer_builder.h"\n'+source[:match.start()]+body+source[match.end():]
