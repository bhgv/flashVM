"""Decline unchecked pointer-list lowering; audited hosted adapters may replace it."""
import hashlib
import re


# source owns the hosted shared layout header. Refuse unreviewed source drift;
# replace only the pointer-cons validator with an unconditional decline. Native
# callers then use existing C opcode fallback before allocation/publication.
# This is not a change to the serialized ISA, pinned source or other leaf kinds.
def adapt(source):
    pattern = r'static int\nleafpointerconslayout\(LeafLayout \*a\)\n\{.*?\n\}'
    matches = list(re.finditer(pattern, source, re.DOTALL))
    if len(matches) != 1:
        raise ValueError('pointer-cons layout boundary drift')
    match = matches[0]
    if hashlib.sha256(match.group().encode()).hexdigest() != EXPECTED:
        raise ValueError('pointer-cons layout changed; review native count protocol')
    replacement = '''static int
leafpointerconslayout(LeafLayout *a)
{
    /* Hosted counts require checked RMW; decline before allocating a node. */
    (void)a;
    return 0;
}'''
    return source[:match.start()]+replacement+source[match.end():]


EXPECTED = '3b58efbbdf41eb0e7a8df74efe50d27e489a462a3afef2d3d43f20e6bf11116e'  # Reviewed pinned validator hash, not an ABI version.
