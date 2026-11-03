"""Actual integer arithmetic tails with cache/return-PC/error controls."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_arithmetic import OPS, GUEST


def wrapper():
    """Specialize the actual native-tail oracle, preserving production emission."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    selected = ' && '.join('opcode != I'+op.upper() for op in OPS)
    index = ' : '.join('opcode == I'+op.upper()+' ? '+str(i) for i, op in enumerate(OPS[:-1]))+' : '+str(len(OPS)-1)
    code = code[:start]+'    if ('+selected+')\n    {\n        return __real_dvm_runtime_bytes_fast(registers, opcode);\n    }\n    int index = '+index+';'+code[end:]
    code = code.replace('entries[5]', 'entries['+str(len(OPS))+']')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]',
                        ' && '.join('entries['+str(i)+']' for i in range(len(OPS))))
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', 'CONTROL >= 100 ? CONTROL - 100 : '+str(len(OPS)-1))
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL >= 100 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('WORD negative = -1;', 'LONG zero = 0;')
    code = code.replace('registers->m = &negative;', 'registers->s = &zero;').replace('IMOVM', 'IDIVL')
    code = code.replace('five guarded entries reached', 'current guarded entries reached')
    code = code.replace('five guarded byte tails', 'current guarded arithmetic tails')
    return code.replace('actual MOVM', 'actual DIVL').replace('negative-count C MOVM', 'zero-divisor C DIVL')


if __name__ == '__main__':
    variants = (9, *range(100, 100+len(OPS)), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS arithmetic24', variants,
         'current guarded entries reached', 'current actual arithmetic tails')
