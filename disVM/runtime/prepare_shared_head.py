"""Adapt only the reviewed HEADMP body, preserving the default C path."""
import hashlib
import re

EXPECTED = 'c49cf2ea516fa23d0376b0967d0088dfbb6c0bd2d0ed0521414409e4a3c54e48'  # Complete final native body.


def adapt(source):
    """Use original index/type validators in process-live C; reject drift first."""
    matches = list(re.finditer(r'OP\(headmp\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('guarded HEADMP source drift')
    body = matches[0].group()
    anchor = '\tl = listhead();'
    if body.count(anchor) != 1:
        raise ValueError('guarded HEADMP selection boundary drift')
    body = body.replace(anchor, '\tif(dvm_runtime_guarded_copies) {\n\t\tdvm_runtime_guarded_headmp(dvm_headmp_type);\n\t\treturn;\n\t}\n'+anchor)
    resolver = '''/* Retained metadata/error precedence, invoked with the source list rooted. */
static Type *
dvm_headmp_type(void *module, const void *cell)
{
    WORD index;
    Type *type;
    if(cell == nil || cell == H || (uintptr)cell > (uintptr)-1-sizeof(index)) {
        error(exBounds);
    }
    memmove(&index, cell, sizeof(index));
    type = moduletype(module, index);
    (void)mappedcopytype(type);
    return type;
}


'''
    match = matches[0]
    return '#include "shared_head.h"\n'+source[:match.start()]+resolver+body+source[match.end():]
