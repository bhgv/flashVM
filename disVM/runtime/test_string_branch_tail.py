"""Six actual string conditional tails: PC publication and retired-code safety."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_string_branch import GUEST

OPS = ('beqc', 'bnec', 'bltc', 'blec', 'bgtc', 'bgec')  # Existing six string operations.


def wrapper():
    """Bind the existing cache/error/return-PC oracle to conditional branches."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    selected = ' && '.join('opcode != I'+op.upper() for op in OPS)
    index = ' : '.join('opcode == I'+op.upper()+' ? '+str(i) for i, op in enumerate(OPS[:-1]))+' : 5'
    code = code[:start]+'    if ('+selected+')\n    {\n        return __real_dvm_runtime_bytes_fast(registers, opcode);\n    }\n    int index = '+index+';'+code[end:]
    code = code.replace('entries[5]', 'entries[6]')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]',
                        ' && '.join('entries['+str(i)+']' for i in range(6)))
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', 'CONTROL >= 100 ? CONTROL - 100 : 5')
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL >= 100 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('registers->m = &negative;', 'registers->s = H;').replace('IMOVM', 'IBNEC')
    code = code.replace('five guarded entries reached', 'six guarded entries reached')
    code = code.replace('five guarded byte tails', 'six guarded branch tails')
    return code.replace('actual MOVM', 'actual BNEC').replace('negative-count C MOVM', 'invalid-source C BNEC')


if __name__ == '__main__':
    variants = (9, *range(100, 106), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS string branches6', variants, 'six guarded entries reached', 'six actual branch tails')
