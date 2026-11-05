"""Route selected typed CONSMP through guarded metadata and graph snapshots."""
import hashlib
import re

EXPECTED = 'b39b725fa1acdf3fe9d7c5879258e36a368c692576dc37d9a49881819e03ebe4'  # Exact native body before this startup adapter.


def adapt(source):
    """Return changed hosted view; refuse drift/double application before write."""
    matches = list(re.finditer(r'OP\(consmp\)\n\{.*?\n\}',source,re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('typed CONSMP source drift')
    match = matches[0]
    body = match.group()
    at = body.index('\n{')+2
    selection = r"""
    if(dvm_runtime_guarded_copies) {
        int selected = dvm_runtime_shared_mode();
        if(!selected) {
            error(exHeap);
        }
        if(selected == 2) {
            WORD captured;
            Type *type;
            const char *problem = dvm_runtime_bytes_snapshot(DVM_R.m, &captured, sizeof(captured));
            if(problem) {
                error((char *)problem);
            }
            type = cconsmappedtype(DVM_R.M, captured);
            dvm_runtime_mapped_cons(DVM_R.s, DVM_R.d, type);
            return;
        }
    }
"""
    body = body[:at]+selection+body[at:]
    return '#include "mapped_cons.h"\n#include "shared_access.h"\n'+source[:match.start()]+body+source[match.end():]
