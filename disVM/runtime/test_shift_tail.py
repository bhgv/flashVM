"""Six actual Limbo-emitted shift tails; LSR is native/dispatch coverage only."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_shift import GUEST

OPS = ('shlb', 'shlw', 'shll', 'shrb', 'shrw', 'shrl')  # Actual compiled guest variants.


def wrapper():
    """Bind the existing native-tail ABI/cache oracle to six production shifts."""
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
    code = code.replace('registers->m = &negative;', 'registers->s = &negative;').replace('IMOVM', 'ISHRL')
    code = code.replace('five guarded entries reached', 'six guarded entries reached')
    code = code.replace('five guarded byte tails', 'six guarded shift tails')
    return code.replace('actual MOVM', 'actual SHRL').replace('negative-count C MOVM', 'invalid-count C SHRL')


if __name__ == '__main__':
    variants = (9, *range(100, 106), 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS shifts6', variants, 'six guarded entries reached', 'six actual shift tails')
