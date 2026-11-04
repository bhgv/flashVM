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
    code = code.replace('static int stage;', 'static unsigned long operation_calls; /* Genuine guarded arithmetic calls. */\nstatic int stage;')
    # The final target-only return must complete dispatch and invoke exactly one
    # genuine arithmetic helper. Entering a tail followed by fallback is insufficient.
    before = '    return __real_dvm_runtime_bytes_fast(registers, opcode);'
    position = code.rfind(before)
    if position < 0:
        raise RuntimeError('target dispatch return drift')
    after = '    unsigned long previous = operation_calls;\n    int complete = __real_dvm_runtime_bytes_fast(registers, opcode);\n    check(complete == 1 && operation_calls == previous+1,"dispatch executes exactly once");\n    return complete;'
    code = code[:position]+code[position:].replace(before,after,1)
    code += r'''


/* Count actual guarded arithmetic execution, not entry into a declining tail.
 * Arguments are independently live borrowed cells; return the real diagnostic.
 * No guard/owner/callback is inserted here; VM errors retain their real unwind. */
const char *__real_dvm_runtime_real_arithmetic(int,const void *,const void *,void *);
const char *__wrap_dvm_runtime_real_arithmetic(int opcode,const void *a,const void *b,void *out)
{
    operation_calls++;
    return __real_dvm_runtime_real_arithmetic(opcode,a,b,out);
}
'''
    return code.replace('actual MOVM', 'actual DIVF').replace('negative-count C MOVM', 'invalid-source C DIVF')


if __name__ == '__main__':
    variants = (9, *range(100, 104), 12, 13)
    controls = {20+3*i+result: ('I'+op.upper(),op,result) for i,op in enumerate(OPS) for result in (0,1,2)}
    if '--positive-only' not in sys.argv:
        variants += (10, 11, *controls)
    main(GUEST, wrapper(), 'PASS REAL arithmetic4', variants, 'four guarded entries reached', 'four actual REAL arithmetic tails',
         extra_symbols=('dvm_runtime_real_arithmetic',), dispatcher_controls=controls)
