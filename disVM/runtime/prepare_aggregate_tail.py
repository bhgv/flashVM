"""Tail-enter actual aggregate C opcodes from reviewed hosted x64 profiles."""
import hashlib
import re

EXPECTED = {  # Exact original emitter functions, not serialized ABI versions.
    'x64mapped': 'abe326d6399850016d47747fafbd4913625fa546e3e1c778591f1f2cfa1076cd',
    'x64mappedcons': 'ebe873b5b3ba8c64536347517b0a119b4453289305b6cfe3f6360f4eb90fd9b5',
    'x64rawcons': 'a416444890848a72eb06488306bc07d9cb63f921552a0a726980a5a22f6a75e0',
}


def adapt(source):
    """Bind host REG/Heap/Type geometry before emitting process-live C targets.

    Return changed owned text or reject drift/repeated application. No new shared
    layout/ID; other backends stay unchanged. C opcodes retain error semantics.
    """
    prefix = '''#include "aggregate_fast.h"

/* Bind actual native metadata; values do not prove operand lifetime/access. */
static int
x64aggregateprofile(LeafLayout *a)
{
    return leafownergclayout(a) && a->wordbytes == sizeof(WORD) &&
        a->s == O(REG, s) && a->d == O(REG, d) && a->m == O(REG, m) &&
        a->module == O(REG, M) && a->listdata == O(List, data) && a->listtail == O(List, tail) &&
        a->heapref == sizeof(Heap)-O(Heap, ref) && a->heapcolor == sizeof(Heap)-O(Heap, color) &&
        a->heaptype == sizeof(Heap)-O(Heap, t) && a->typeref == O(Type, ref) &&
        a->typesize == O(Type, size) && a->typenp == O(Type, np) &&
        a->mutator == &mutator && a->nprop == &nprop && a->markcolor == propagator;
}
'''
    for name in EXPECTED:
        args = 'X64Buf *b, LeafLayout *a'+(', LeafOp *p' if name == 'x64mapped' else '')
        pattern = r'static int\n'+name+r'\('+re.escape(args)+r'\)\n\{.*?\n\}'
        matches = list(re.finditer(pattern, source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED[name]:
            raise ValueError('x64 aggregate emitter drift: '+name)
        if name == 'x64mapped':
            select = '''    if(p->alu == 0) {
        entry = (unsigned long long)dvm_runtime_aggregate_movmp_fast;
    }else if(p->alu == 1) {
        entry = (unsigned long long)dvm_runtime_aggregate_headmp_fast;
    }else{
        return 0;
    }'''
        else:
            operation = 'consmp' if name == 'x64mappedcons' else 'consm'
            select = '    entry = (unsigned long long)dvm_runtime_aggregate_'+operation+'_fast;'
        body = '''static int
%s(%s)
{
    unsigned long long entry;
    if(!x64aggregateprofile(a)) {
        return 0;
    }
%s
    x64byte(b, 0x48);
    x64byte(b, 0xb8);
    x64word(b, entry);
    x64word(b, entry>>32);
    /* Original C return address stays on stack; no RX return PC survives. */
    x64byte(b, 0xff);
    x64byte(b, 0xe0);
    return 1;
}'''%(name,args,select)
        match = matches[0]
        source = source[:match.start()]+body+source[match.end():]
    # Profile definition follows LeafLayout declarations from the including unit.
    anchor = '#include "leaf64-ops.h"'
    if source.count(anchor) != 1:
        raise ValueError('aggregate profile declaration boundary drift')
    return source.replace(anchor, anchor+'\n'+prefix, 1)
