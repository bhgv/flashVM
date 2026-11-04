"""Actual LENL tail: continuous-owner handler and retired-code/error safety."""
import sys
from test_bytes_tail import main, WRAPPER
from test_shared_list_length import GUEST


def wrapper():
    """Bind the inherited ABI/cache/error oracle to exactly one real LENL entry."""
    code = WRAPPER
    start = code.index('    if (opcode != IMOVB')
    end = code.index('\n    Module *module', start)
    code = code[:start]+'''    if (opcode != ILENL)
    {
        return __real_dvm_runtime_bytes_fast(registers, opcode);
    }
    int index = 0;'''+code[end:]
    code = code.replace('entries[5]', 'entries[1]')
    code = code.replace('entries[0] && entries[1] && entries[2] && entries[3] && entries[4]', 'entries[0]')
    code = code.replace('CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1', '0')
    code = code.replace('CONTROL <= 4 || CONTROL == 13', 'CONTROL == 0 || CONTROL == 13')
    code = code.replace('if (CONTROL >= 12)', 'if (CONTROL == 12 || CONTROL == 13)')
    code = code.replace('registers->m = &negative;', 'registers->s = H;').replace('IMOVM', 'ILENL')
    code = code.replace('five guarded entries reached', 'one list-length entry reached')
    code = code.replace('five guarded byte tails', 'one actual list-length tail')
    return code.replace('actual MOVM', 'actual LENL').replace('negative-count C MOVM', 'invalid-source C LENL')


if __name__ == '__main__':
    variants = (9, 0, 12, 13)
    if '--positive-only' not in sys.argv:
        variants += (10, 11)
    main(GUEST, wrapper(), 'PASS list length32', variants, 'one list-length entry reached', 'one actual LENL tail')
