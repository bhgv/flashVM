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
