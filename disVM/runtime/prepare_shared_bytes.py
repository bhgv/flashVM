"""Adapt reviewed hosted scalar/raw copies and SELF without changing file opcodes."""
import hashlib
import re

EXPECTED = {'movpc': 'f07ded6c0a0e377194bbd09b6378d47bc24f408c8a84e2fab928dbaee34730d7', 'lea': '80776b107df2ae1c2268afcc271b9a6db20fcd6e9f71b5c65e9eeb970706cd62', 'movb': '979897912a693e4d8936c16e4bcce54e57edaf281ab17a3805358ecceeb2e315', 'movw': '3618c2035d67ab030e5fa0552464bf28970023778fc66e0061da63e4902c5ee1', 'movl': '7c8de14452ad8b14ef03d205fa0adf97995cfba001c96ed77f8cca004f391f78', 'movf': '8eb3aa0ff49976b6e75e80b4ff84a47d542e3c2eda24627818f56c35e4eb53cb', 'movm': '5fa9e16c92e2ff10b100dcac76c941a5213101f45651217f60c0d5d7354ebf2c', 'self': '5dbb1295c45b134fdba5acccdbd19191efe7094431af8fc7ec077ee802f6cd11'}  # Complete final reviewed bodies, after count/snapshot views.


def adapt(source):
    """Return owned source or reject drift/repeated adaptation before publication.

    Default byte copies retain their original machine/C path. Explicit guarded
    profile routes five copy opcodes through range helpers; SELF always uses
    guarded MOVP under worker worlds. All errors/destructors occur outside guards.
    """
    for name in EXPECTED:
        pattern = r'OP\('+name+r'\)\n\{.*?\n\}'
        matches = list(re.finditer(pattern, source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED[name]:
            raise ValueError('hosted shared byte source drift: '+name)
        body = matches[0].group()
        if name == 'self':
            body = '''OP(self)
{
    Modlink *module;
    void *previous;
    int replay;
    const char *problem;
    if(DVM_R.d == nil || DVM_R.d == H || (uintptr)DVM_R.d > (uintptr)-1-sizeof(module)) {
        error(exHeap);
    }
    module = DVM_R.M;
    if(module == nil || module == H) {
        error(exModule);
    }
    /* Current REG.M is context-owned and rooted throughout this opcode. */
    problem = dvm_runtime_movp_opcode(ISELF, &DVM_R.M, DVM_R.d, &previous, &replay);
    if(replay) {
        return;
    }
    if(problem != nil) {
        error((char *)problem);
    }
    destroy(previous);
}'''
        elif name in ('lea', 'movpc'):
            value = 'address' if name == 'lea' else 'target'
            if name == 'movpc':
                capture = 'memmove(&pc, DVM_R.s, sizeof(pc));'
                if body.count(capture) != 1:
                    raise ValueError('hosted MOVPC source capture drift')
                body = body.replace(capture, guarded_snapshot('pc', capture, 'DVM_R.s', retry_opcode='IMOVPC'))
            publication = f'memmove(DVM_R.d, &{value}, sizeof({value}));'
            if body.count(publication) != 1:
                raise ValueError('hosted address publication drift: '+name)
            body = body.replace(publication, guarded_publish(value, 'DVM_R.d', publication, 'I'+name.upper()))
        else:
            if name == 'movm':
                capture = 'memmove(&n, DVM_R.m, sizeof(n));'
                if body.count(capture) != 1:
                    raise ValueError('hosted MOVM count capture drift')
                body = body.replace(capture, guarded_snapshot('n', capture, retry_opcode='IMOVM'))
            size = {'movb':'sizeof(BYTE)', 'movw':'sizeof(WORD)', 'movl':'sizeof(LONG)',
                    'movf':'sizeof(REAL)', 'movm':'n'}[name]
            statement = 'memmove(DVM_R.d, DVM_R.s, '+size+');'
            if body.count(statement) != 1:
                raise ValueError('hosted byte payload boundary drift: '+name)
            body = body.replace(statement, '''if(dvm_runtime_guarded_copies) {
            const char *problem = dvm_runtime_bytes_copy_opcode(I'''+name.upper()+''', DVM_R.s, DVM_R.d, '''+size+''');
            if(problem != nil) {
                error((char *)problem);
            }
        } else {
            '''+statement+'''
        }''')
        source = source[:matches[0].start()]+body+source[matches[0].end():]
    return '#include "shared_bytes.h"\n#include "shared_access.h"\n#include "shared_pointer.h"\n'+source


def guarded_snapshot(variable, fallback, source="DVM_R.m", retry_opcode=None):
    """Capture one scalar on the guarded path, with the original serial read."""
    if retry_opcode:
        return f'''if(dvm_runtime_guarded_copies) {{
        DVM_CellResult claim;
        const char *problem = dvm_runtime_bytes_snapshot_attempt({source}, &{variable}, sizeof({variable}), &claim);
        if(claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry({retry_opcode})) {{
            return;
        }}
        if(problem != nil) {{
            error((char *)problem);
        }}
    }} else {{
        {fallback}
    }}'''
    return f'''if(dvm_runtime_guarded_copies) {{
        const char *problem = dvm_runtime_bytes_snapshot({source}, &{variable}, sizeof({variable}));
        if(problem != nil) {{
            error((char *)problem);
        }}
    }} else {{
        {fallback}
    }}'''


def guarded_publish(variable, destination, fallback, opcode):
    """Publish a private scalar through the shared byte-copy guard."""
    return f'''if(dvm_runtime_guarded_copies) {{
        const char *problem = dvm_runtime_bytes_publish_opcode({opcode}, &{variable}, {destination}, sizeof({variable}));
        if(problem != nil) {{
            error((char *)problem);
        }}
    }} else {{
        {fallback}
    }}'''
