"""Actual four Limbo-emitted integer tails using the shared native ABI oracle.

SHORT operations are covered by the native six-op C suite, not this guest.
"""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_integer import GUEST


def wrapper():
    """Bind actual integer entries to the shared cache/return-PC/error oracle."""
    code = WRAPPER
    for old, new in [('IMOVB', 'ICVTBW'), ('IMOVW', 'ICVTWB'), ('IMOVL', 'ICVTWL'),
                     ('IMOVF', 'ICVTLW'), ('IMOVM', 'ICVTLW')]:
        code = code.replace(old, new)
    first = code.index('    int index = opcode == ')
    last = code.index('\n', first)
    code = code[:first]+'''    if (opcode != ICVTBW && opcode != ICVTWB && opcode != ICVTWL && opcode != ICVTLW)
    {
        return __real_dvm_runtime_bytes_fast(registers, opcode);
    }
    int index = opcode == ICVTBW ? 0 : opcode == ICVTWB ? 1 : opcode == ICVTWL ? 2 : 3;'''+code[last:]
    code = code.replace('entries[5]', 'entries[4]').replace(' && entries[4]', '')
    code = code.replace('CONTROL <= 4', 'CONTROL <= 3').replace('CONTROL >= 12 ? 4', 'CONTROL >= 12 ? 3')
    code = code.replace('registers->m = &negative;', 'registers->s = H;')
    code = code.replace('five guarded entries reached', 'four guarded entries reached')
    code = code.replace('five guarded byte tails', 'four guarded integer tails')
    return code.replace('actual MOVM', 'actual CVTLW').replace('negative-count C MOVM', 'invalid-source C CVTLW')


if __name__ == '__main__':
    variants = (9, 0, 1, 2, 3, 12, 13) if '--positive-only' in sys.argv else (9, 0, 1, 2, 3, 12, 13, 10, 11)
    main(GUEST, wrapper(), 'PASS integer conversions64', variants,
         'four guarded entries reached', 'four actual integer tails')
