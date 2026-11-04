"""Select guarded local record constructors after validating the retained bodies."""
import hashlib
import re

EXPECTED = {'new': '5de4e3aa3cd1b9cf998676decf15c972e05d79a7b8a0e70e63a6314d2dbb2f98', 'newz': '77da78be5cd19e9925128b9f8f7f598b388cd7aeb233dc30edd83ff63dc08125'}  # SHA256 of each exact adapted opcode body.


def adapt(source):
    """Insert worker-mode dispatch or raise on source drift."""
    for name, digest in EXPECTED.items():
        match = re.search(r"OP\(" + name + r"\)\n\{.*?\n\}", source, re.S)
        if not match or hashlib.sha256(match.group().encode()).hexdigest() != digest:
            raise ValueError("record constructor drift: " + name)
        body = match.group()
        at = body.index("\n{") + 2
        clear = 1 if name == "newz" else 0
        selection = f"""
    if (dvm_runtime_guarded_copies)
    {{
        int selected = dvm_runtime_shared_mode();
        if (!selected)
        {{
            error(exHeap);
        }}
        if (selected == 2)
        {{
            WORD captured;
            const char *problem = dvm_runtime_bytes_copy(DVM_R.s, &captured, sizeof(captured));
            if (problem)
            {{
                error((char *)problem);
            }}
            dvm_runtime_record_new(DVM_R.d, moduletype(DVM_R.M, captured), {clear});
            return;
        }}
    }}
"""
        source = source[:match.start()] + body[:at] + selection + body[at:] + source[match.end():]
    return '#include "record_new.h"\n#include "shared_access.h"\n' + source
