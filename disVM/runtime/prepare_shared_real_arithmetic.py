"""Select guarded REAL arithmetic only after complete-body drift checks."""
import hashlib
import re

EXPECTED = {'negf': 'a72d42cb76d14c6933e37974887c2e6d234a8ab25512a26429050bfb20dc364e', 'addf': '8d2335928bfe176ad941fede3473b668ca411a8fbd9fcb58b36dbb3141e5ac69', 'subf': 'f96409cdb8cb6a947a988afc21923e34bda29578e285f3687dad6dac89f2826d', 'mulf': 'b43708a46e4ce10f6694c6fb7d88d2c12c44d0f3e667d4197a5b74f0df674c08', 'divf': 'b8ddd42771ce2880ea450f95446f0def4ad4124332c38de49af4d811fc7f4a32'}  # Complete retained native operation bodies.


def adapt(source):
    """Borrow source text; preserve default path and refuse missing/repeated/drifted bodies."""
    for name, expected in EXPECTED.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded REAL arithmetic source drift: '+name)
        match = matches[0]
        body = match.group()
        anchor = '\t/* Reject malformed slots' if name == 'negf' else '\t/* Bound every used slot'
        operands = 'DVM_R.s, nil' if name == 'negf' else 'DVM_R.m, DVM_R.s'
        if body.count(anchor) != 1:
            raise ValueError('guarded REAL arithmetic insertion drift: '+name)
        code = '\tif(dvm_runtime_guarded_copies) {\n\t\tconst char *problem = dvm_runtime_real_arithmetic(I'+name.upper()+', '+operands+', DVM_R.d);\n\t\tif(problem != nil) {\n\t\t\terror((char *)problem);\n\t\t}\n\t\treturn;\n\t}\n'
        source = source[:match.start()]+body.replace(anchor,code+anchor)+source[match.end():]
    source = adapt_scaled(source)
    return source.replace('#include "interp.h"', '#include "interp.h"\n#include "shared_real_arithmetic.h"', 1)


SCALED_BODIES = {
    'mulx0': 'e28e1014fd154385287a364417962c586baf4077c10ce8ecfb7803da74a70039',
    'divx0': '40924710ce14e815241744c5b847636695f122b0e28ac170e16945c0d280031f',
    'cvtxx0': '932321bb0d92975b2c68a79c1554f6f4ce71ae3d1a3ec6fe9de3c9588041fec4',
    'mulx1': '371138e593f2de08da59463b80191a9f0b749cd516881bf42600871c8b63df6e',
    'divx1': '40ad40a10e83edf6f801abfef51e705700f41cec40f7de5d8d468c6af08a5e26',
    'cvtxx1': '8382a8168111b1eefa146a73ac306c4a8df63cb3ff1330a20303428573c8b3eb',
    'divx': 'c0450aa55a901748c31addef7571e3f4528b9f7bcbc2cb4e72f76116bd97d596',
    'mulx': '661033150c6df95bad861e760b9a4bcfcd117d4b219cd18b3bd2998843143503',
    'cvtfx': '556d86b5077cc0edeb7158299dc39de0508279992c007e3fa70f963c30cea729',
    'cvtxf': '6cc88fe0e48db1df1c75406bc2bdb4d4fdd04afe6e00bac0c67b284a84d4c0bd',
    'cvtxx': '8248f8681622d3c68b081d0f140fcbdd6bd7ecba244c9ed8b33948511fe6c383',
    'iexpw': '773e9d241de8978b312eeec4a2ea82125a46dd2e058ad4593b9df279eb5199e3',
    'iexpl': '823bd5302da4881f57ed2456e06f16798025cc1dd53e0397db6cdc352ee35fa9',
    'iexpf': 'fc9dc2fb36b83befec00ce8190c531af880acd4ccb53326cac4e23a00a33b84f',
}


def adapt_scaled(source):
    """Use the original native evaluator for both direct and guarded slots."""
    for name, expected in SCALED_BODIES.items():
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}', source, re.S))
        if name == 'cvtxx':
            # The upstream file also retains a commented floating-point alternative.
            matches = [match for match in matches if '\tWORD p, value, result;' in match.group()]
        if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != expected:
            raise ValueError('guarded scaled arithmetic source drift: '+name)
        match = matches[0]
        slots = name+'_slots'
        fixed_product = name in ('mulx', 'divx')
        fixed_variant = name.endswith(('0', '1'))
        prefix = 'WORD scale, ' if fixed_product else 'const WORD *coefficients, '
        body = match.group().replace('OP('+name+')',
            'static const char *\n'+slots+'('+prefix+'const void *first_slot, const void *second_slot, void *result_slot)')
        body = body.replace('DVM_R.m', 'first_slot').replace('DVM_R.s', 'second_slot').replace('DVM_R.d', 'result_slot')
        if fixed_product:
            body = body.replace('p = fixedword((NREG+2)*IBY2WD);', 'p = scale;')
        if fixed_variant:
            body = body.replace('fixedword(NREG*IBY2WD)',
                '(coefficients ? coefficients[0] : fixedword(NREG*IBY2WD))')
            body = body.replace('fixedword((NREG+2)*IBY2WD)',
                '(coefficients ? coefficients[1] : fixedword((NREG+2)*IBY2WD))')
            body = re.sub(r'(\t)([rs] = fixedshift\([rs], p\);)',
                r'\1if(p < -63 || p > 63)\n\1\treturn exInval;\n\1\2', body)
            body = body.replace('w = fixedroundmask(p);',
                '{\n\t\t\tif(p >= 0 || p < -63)\n\t\t\t\treturn exInval;\n'
                '\t\t\tw = fixedroundmask(p);\n\t\t}')
        elif not fixed_product:
            body = body.replace('\n{\n', '\n{\n    (void)coefficients;\n', 1)
        # Slot-level diagnostics must return to the caller before VM unwinding.
        body = re.sub(r'error\((ex[A-Za-z]+)\);', r'return \1;', body)
        body = body.replace('return;', 'return NULL;')
        body = body[:-1]+'    return NULL;\n}'
        init = '    WORD scale = fixedword((NREG+2)*IBY2WD);\n' if fixed_product else ''
        args = ('scale, ' if fixed_product else '')+'DVM_R.m, DVM_R.s, DVM_R.d'
        helper = 'dvm_runtime_fixed_product' if fixed_product else 'dvm_runtime_scaled_arithmetic'
        opcode = ('' if name.startswith('iexp') else 'I')+name.upper()
        direct_args = args if fixed_product else 'NULL, '+args
        wrapper = f'''
OP({name})
{{
    const char *problem;
{init}    if(dvm_runtime_guarded_copies) {{
        problem = {helper}({opcode}, {args}, {slots});
    }} else {{
        problem = {slots}({direct_args});
    }}
    if(problem != nil) {{
        error((char*)problem);
    }}
}}
'''
        source = source[:match.start()]+body+'\n'+wrapper+source[match.end():]
    return source
