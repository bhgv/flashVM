"""Route the reviewed MOVMP body through an opt-in mapped transaction."""
import hashlib
import re

EXPECTED = '2bb3a55414025b3f5d79e81d6b3eb06c57fe4beddc233051986b63748f4ce2c6'  # Complete final reviewed body.


def adapt(source):
    """Preserve original validation/default path; reject source drift before emit."""
    matches = list(re.finditer(r'OP\(movmp\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('hosted mapped-copy body drift')
    body = matches[0].group()
    anchor = '\tsource = DVM_R.s;\n\tdestination = DVM_R.d;'
    if body.count(anchor) != 1:
        raise ValueError('hosted mapped-copy publication boundary drift')
    body = body.replace(anchor, anchor+\
        '\n\tif(dvm_runtime_guarded_copies) {\n\t\tdvm_runtime_mapped_copy(source, destination, t, size);\n\t\treturn;\n\t}')
    return '#include "shared_mapped.h"\n'+source[:matches[0].start()]+body+source[matches[0].end():]
