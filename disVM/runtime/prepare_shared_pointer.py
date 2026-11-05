"""Replace only the reviewed final hosted MOVP body with guarded copy/retirement."""
import hashlib
import re

EXPECTED = "f1ff6651716028e5c1000b9ed6527dd2ba8ec968fba0bce17a1b3de63525a08d"  # Complete reviewed final MOVP, including count/mark adaptations.


def adapt(source):
    """Return private owned C view or refuse drift/repeated application.

    Destroy stays outside the native cell-guard interval; error handling cannot
    resume a declined operation after ownership/publication effects.
    """
    matches = list(re.finditer(r'OP\(movp\)\n\{.*?\n\}', source, re.S))
    if len(matches) != 1 or hashlib.sha256(matches[0].group().encode()).hexdigest() != EXPECTED:
        raise ValueError('hosted shared MOVP body drift')
    replacement = '''OP(movp)
{
    void *previous;
    int replay;
    const char *problem = dvm_runtime_movp_opcode(IMOVP, DVM_R.s, DVM_R.d, &previous, &replay);
    if(replay) {
        return;
    }
    if(problem != nil) {
        error((char *)problem);
    }
    destroy(previous);
}'''
    match = matches[0]
    return '#include "shared_pointer.h"\n'+source[:match.start()]+replacement+source[match.end():]
