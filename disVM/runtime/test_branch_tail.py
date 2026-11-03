"""Eighteen actual conditional tails: PC publication and retired-code safety."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_branch import GUEST

OPS = ('beqb', 'bneb', 'bltb', 'bleb', 'bgtb', 'bgeb', 'beqw', 'bnew', 'bltw', 'blew', 'bgtw', 'bgew', 'beql', 'bnel', 'bltl', 'blel', 'bgtl', 'bgel')  # All actual native branch variants expected from this guest.


def wrapper():
    """Bind the existing cache/error/return-PC oracle to conditional branches."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    selected = ' && '.join('opcode != I'+op.upper() for op in OPS)
    index = ' : '.join('opcode == I'+op.upper()+' ? '+str(i) for i, op in enumerate(OPS[:-1]))+' : 17'
    code = code[:start]+'    if ('+selected+')\n    {\n        return __real_dvm_runtime_bytes_fast(registers, opcode);\n    }\n    int index = '+index+';'+code[end:]
    code = code.replace('entries[5]', 'entries[18]')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]',
                        ' && '.join('entries['+str(i)+']' for i in range(18)))
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', 'CONTROL >= 100 ? CONTROL - 100 : 17')
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL >= 100 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('registers->m = &negative;', 'registers->s = H;').replace('IMOVM', 'IBNEW')
    code = code.replace('five guarded entries reached', 'eighteen guarded entries reached')
    code = code.replace('five guarded byte tails', 'eighteen guarded branch tails')
    return code.replace('actual MOVM', 'actual BNEW').replace('negative-count C MOVM', 'invalid-source C BNEW')


if __name__ == '__main__':
    variants = (9, *range(100, 118), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS branches18', variants, 'eighteen guarded entries reached', 'eighteen actual branch tails')
