"""Four actual REAL arithmetic tails: cache/error and return-PC safety."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_real_arithmetic import GUEST

OPS = ('addf', 'subf', 'mulf', 'divf')  # Existing four REAL arithmetic operations.


def wrapper():
    """Bind the existing cache/error/return-PC oracle to binary REAL arithmetic."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    selected = ' && '.join('opcode != I'+op.upper() for op in OPS)
    index = ' : '.join('opcode == I'+op.upper()+' ? '+str(i) for i, op in enumerate(OPS[:-1]))+' : 3'
    code = code[:start]+'    if ('+selected+')\n    {\n        return __real_dvm_runtime_bytes_fast(registers, opcode);\n    }\n    int index = '+index+';'+code[end:]
    code = code.replace('entries[5]', 'entries[4]')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]',
                        ' && '.join('entries['+str(i)+']' for i in range(4)))
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', 'CONTROL >= 100 ? CONTROL - 100 : 3')
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL >= 100 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('registers->m = &negative;', 'registers->s = H;').replace('IMOVM', 'IDIVF')
    code = code.replace('five guarded entries reached', 'four guarded entries reached')
    code = code.replace('five guarded byte tails', 'four guarded REAL tails')
    return code.replace('actual MOVM', 'actual DIVF').replace('negative-count C MOVM', 'invalid-source C DIVF')


if __name__ == '__main__':
    variants = (9, *range(100, 104), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS REAL arithmetic4', variants, 'four guarded entries reached', 'four actual REAL arithmetic tails')
