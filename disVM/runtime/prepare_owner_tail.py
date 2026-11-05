"""Replace reviewed x64 inline owner RMW/marking with checked C tail entries."""
import hashlib
import re

EXPECTED = 'b887bd969c7bddeac3bbb842b4e6ec0fff13cb4ef232740e075a5fac89f85ccd'  # Reviewed original x64owner emitter.


def adapt(source):
    """Return a private hosted emitter, rejecting drift/repeated adaptation.

    Bind actual host offsets/GC identities before emission; no new shared layout.
    SysV REG remains in rdi, original C return PC remains on stack. Other target
    emitters are untouched, retaining serialized execution and separate proofs.
    """
    matches = list(re.finditer(r'static int\nx64owner\(X64Buf \*b, LeafLayout \*abi, LeafOp \*p\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('x64 owner emitter changed; review checked C tail ABI')
    match = matches[0]
    body = '''static int
x64owner(X64Buf *b, LeafLayout *abi, LeafOp *p)
{
    unsigned long long entry;
    if(!leafownergclayout(abi) || abi->wordbytes != sizeof(WORD) ||
       abi->s != O(REG, s) || abi->d != O(REG, d) || abi->module != O(REG, M) ||
       abi->heapref != sizeof(Heap)-O(Heap, ref) ||
       abi->heapcolor != sizeof(Heap)-O(Heap, color) ||
       abi->refbytes != sizeof(((Heap*)0)->ref) ||
       abi->mutator != &mutator || abi->nprop != &nprop || abi->markcolor != propagator ||
       (p->alu >= 2 && (abi->listdata != O(List, data) || abi->listtail != O(List, tail)))) {
        return 0;
    }
    switch(p->alu) {
    case 0:
        entry = (unsigned long long)dvm_runtime_owner_movp_fast;
        break;
    case 1:
        entry = (unsigned long long)dvm_runtime_owner_self_fast;
        break;
    case 2:
        entry = (unsigned long long)dvm_runtime_owner_headp_fast;
        break;
    case 3:
        entry = (unsigned long long)dvm_runtime_owner_tail_fast;
        break;
    default:
        return 0;
    }
    x64byte(b, 0x48);
    x64byte(b, 0xb8);
    x64word(b, entry);
    x64word(b, entry>>32);
    /* Tail JMP: original C return address survives cache retirement/errors. */
    x64byte(b, 0xff);
    x64byte(b, 0xe0);
    return 1;
}'''
    return '#include "owner_fast.h"\n'+source[:match.start()]+body+source[match.end():]


ARM64_OWNER_BODIES = {  # Pinned emitter bodies that directly update GC ownership.
    'a64owner': '0ddfd87f9a22f49f77e837f8f17aadbe9ee938c67241edb498be01b5651975d5',
    'a64pointercons': '2e0ee53471e51a4e273a5f1ca675aaf833f782b5365e0d93a8a78b2e307924f0',
    'a64mappedcons': 'b636d1c5a32f779b7ec679245eec18640bdd3625bb2e2aa74b4db627b3682d22',
}


def adapt_arm64(source):
    """Replace reviewed inline ownership writes with AAPCS64 C tails.

    Borrow the pinned emitter text. Return a private hosted source view or
    reject drift. x0 retains REG and LR retains the caller's return address.
    """
    tail = '''/* Emit a checked C tail using x16 as the only scratch register. */
static int
a64ownertail(A64Buf *b, LeafLayout *abi, unsigned long long entry)
{
    int half;
    if(abi->wordbytes != sizeof(WORD) || abi->s != O(REG, s) ||
       abi->d != O(REG, d) || abi->m != O(REG, m) || abi->module != O(REG, M) ||
       !entry || entry%4 != 0) {
        return 0;
    }
    for(half = 0; half < 4; half++) {
        a64put(b, (half == 0 ? 0xd2800010U : 0xf2800010U) |
            half<<21 | ((entry>>(half*16))&65535)<<5);
    }
    a64put(b, 0xd61f0200U); /* br x16 */
    return 1;
}


'''
    bodies = {
        'a64owner': tail+'''static int
a64owner(A64Buf *b, LeafLayout *abi, LeafOp *p)
{
    switch(p->alu) {
    case 0:
        return a64ownertail(b, abi, (unsigned long long)dvm_runtime_owner_movp_fast);
    case 1:
        return a64ownertail(b, abi, (unsigned long long)dvm_runtime_owner_self_fast);
    case 2:
        return a64ownertail(b, abi, (unsigned long long)dvm_runtime_owner_headp_fast);
    case 3:
        return a64ownertail(b, abi, (unsigned long long)dvm_runtime_owner_tail_fast);
    default:
        return 0;
    }
}''',
        'a64pointercons': '''static int
a64pointercons(A64Buf *b, LeafLayout *a)
{
    return a64ownertail(b, a, (unsigned long long)dvm_runtime_pointer_fast);
}''',
        'a64mappedcons': '''static int
a64mappedcons(A64Buf *b, LeafLayout *a)
{
    return a64ownertail(b, a, (unsigned long long)dvm_runtime_aggregate_consmp_fast);
}''',
    }
    for name, digest in ARM64_OWNER_BODIES.items():
        matches = list(re.finditer(r'static int\n'+name+r'\(.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != digest:
            raise ValueError('arm64 ownership emitter drift: '+name)
        match = matches[0]
        source = source[:match.start()]+bodies[name]+source[match.end():]
    return '#include "owner_fast.h"\n#include "pointer_builder.h"\n#include "aggregate_fast.h"\n'+source
