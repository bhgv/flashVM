"""Reuse actual byte-tail ABI controls for five guarded scalar/raw list heads."""
from test_bytes_tail import main, WRAPPER

GUEST = '''implement Heads;
include "sys.m";
Heads: module {init: fn();};
Plain: adt {a: int; b: int;};
init() {
    sys := load Sys Sys->PATH;
    bytes := byte 137 :: nil;
    words := 731 :: nil;
    longs := big 9000000007 :: nil;
    reals := 3.5 :: nil;
    records := Plain(3, 7) :: nil;
    sys->print("BEGIN scalar heads\\n");
    for (i := 0; i < 32; i++) {
        b := hd bytes;
        w := hd words;
        l := hd longs;
        f := hd reals;
        p := hd records;
        if (int b != 137 || w != 731 || l != big 9000000007 ||
            f != 3.5 || p.a != 3 || p.b != 7) {raise "scalar head mismatch";}
    }
    sys->print("PASS scalar heads32\\n");
}
'''  # Actual compiler must reach all five distinct opcodes.


def wrapper():
    """Bind shared native ABI oracle to HEAD opcodes, ignoring other copy tails.

    The actual emitter/helper remain production code. These are diagnostic token
    substitutions, not a modeled JIT or replacement opcode implementation.
    """
    code = WRAPPER
    for name in ('B', 'W', 'L', 'F', 'M'):
        code = code.replace('IMOV'+name, 'IHEAD'+name)
    anchor = '    int index = opcode == IHEADB'
    if code.count(anchor) != 1:
        raise RuntimeError('shared head-tail wrapper drift')
    code = code.replace(anchor, '''    if (opcode != IHEADB && opcode != IHEADW && opcode != IHEADL &&
        opcode != IHEADF && opcode != IHEADM)
    {
        return __real_dvm_runtime_bytes_fast(registers, opcode);
    }
'''+anchor)
    return code.replace("actual MOVM", "actual HEADM").replace("negative-count C MOVM", "negative-count C HEADM")


if __name__ == '__main__':
    main(GUEST, wrapper(), "PASS scalar heads32")
    print("PASS five actual guarded scalar/raw list-head tails")
