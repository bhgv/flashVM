"""Make stack-check recursion invocation-local in the reviewed hosted view."""
import hashlib

EXPECTED = '61071b3379230396cb136b7105ae9b30927a2dd7c957069149ea06035c80d85c'  # Native-register validstk.c before this adaptation.


def adapt(source):
    """Return owned adapted text, rejecting complete-source drift/double use.

    Depth is a value on each C invocation, never process/TLS state. Atomic count
    observation still requires independent source lifetime and managed exclusion.
    """
    if hashlib.sha256(source.encode()).hexdigest() != EXPECTED:
        raise ValueError('hosted stack-check source drift')
    changes = (
        ('static int depth;\n', ''),
        ('void\nmemchk(void *p, Type *t)',
         'static void\nmemchk_depth(void *p, Type *t, unsigned int depth)'),
        ('\tdepth++;\n', ''),
        ('\tdepth--;\n', ''),
        ('if(h->ref <= 0)', 'if(dvm_refcount_load_u(&h->ref) == 0)'),
        ('memchk(v, h->t);', 'memchk_depth(v, h->t, depth+1);'),
    )
    for before, after in changes:
        if source.count(before) != 1:
            raise ValueError('hosted stack-check boundary drift: '+before)
        source = source.replace(before, after)
    # Preserve the old maximum101 visited nodes. Root uses0 and each edge
    # passes depth+1; an entry with depth101 returns before reading payload.
    wrapper = '''

/* [GC:] Check a borrowed live graph under existing managed exclusion.
 * p/t stay independently owned; recursion budget belongs to this invocation.
 * No retain, root, payload permission or collector admission is acquired. */
void
memchk(void *p, Type *t)
{
    memchk_depth(p, t, 0);
}
'''
    anchor = '\n\n/* Follow frame metadata'
    if source.count(anchor) != 1:
        raise ValueError('hosted stack-check wrapper boundary drift')
    source = source.replace(anchor, wrapper+anchor)
    return '#include "disvm/refcount.h"\n'+source
