"""Actual fifteen arithmetic tails with cache/return-PC/error controls."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_arithmetic import OPS, GUEST


def wrapper():
    """Specialize the actual native-tail oracle, preserving production emission."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    selected = ' && '.join('opcode != I'+op.upper() for op in OPS)
    index = ' : '.join('opcode == I'+op.upper()+' ? '+str(i) for i, op in enumerate(OPS[:-1]))+' : 14'
    code = code[:start]+'    if ('+selected+')\n    {\n        return __real_dvm_runtime_bytes_fast(registers, opcode);\n    }\n    int index = '+index+';'+code[end:]
    code = code.replace('entries[5]', 'entries[15]')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]',
                        ' && '.join('entries['+str(i)+']' for i in range(15)))
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', 'CONTROL >= 100 ? CONTROL - 100 : 14')
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL >= 100 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('registers->m = &negative;', 'registers->s = H;').replace('IMOVM', 'IADDL')
    code = code.replace('five guarded entries reached', 'fifteen guarded entries reached')
    code = code.replace('five guarded byte tails', 'fifteen guarded arithmetic tails')
    return code.replace('actual MOVM', 'actual ADDL').replace('negative-count C MOVM', 'invalid-source C ADDL')


if __name__ == '__main__':
    variants = (9, *range(100, 115), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS arithmetic15', variants,
         'fifteen guarded entries reached', 'fifteen actual arithmetic tails')
