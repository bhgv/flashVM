"""Select the rooted channel constructor without modifying the vendor snapshot."""
import hashlib
import re


CHANNELS = (
    (r"Channel\*\nnewc\(Type \*t, void \(\*mover\)\(void\)\)\n\{.*?\n\}",
     "c7fc974386282afcc682a2e207059ed1ad0b3769d5968acd8ca3ffcf1390d548",
     "return dvm_runtime_channel_new(DVM_R.d, t, guarded_capacity(), mover, 0);"),
    (r"OP\(newcm\)\n\{.*?\n\}",
     "7e5ae37285bc157aa61d2c7c85699904f568ea0e01026f003038f19d60483329",
     """WORD size = guarded_channel_word(DVM_R.s);
        if (size < 0)
        {
            error(exNegsize);
        }
        if ((uvlong)size > 0x7fffffff)
        {
            error(exHeap);
        }
        dvm_runtime_raw_channel_new(DVM_R.d, guarded_capacity(), size, movm);
        return;"""),
    (r"OP\(newcmp\)\n\{.*?\n\}",
     "2c512e9f036513e88149c561700a9da4db032c012504a2fe517425d935cfd9ce",
     """WORD index = guarded_channel_word(DVM_R.s);
        Type *type = moduletype(DVM_R.M, index);
        dvm_runtime_channel_new(DVM_R.d, type, guarded_capacity(), movtmp, 0);
        return;"""),
)

HELPERS = r"""
/* Require the same runtime admission as other guarded constructors. */
static int guarded_channel_selected(void)
{
    int mode;
    if (!dvm_runtime_guarded_copies)
    {
        return 0;
    }
    mode = dvm_runtime_shared_mode();
    if (!mode)
    {
        error(exHeap);
    }
    return mode == 2;
}


/* Snapshot native WORD operands before an allocation can call a monitor. */
static WORD guarded_channel_word(void *operand)
{
    WORD value;
    const char *problem = dvm_runtime_bytes_copy(operand, &value, sizeof(value));
    if (problem)
    {
        error((char *)problem);
    }
    return value;
}


/* Dis encodes an absent capacity by aliasing the destination operand. */
static WORD guarded_capacity(void)
{
    return DVM_R.m == DVM_R.d ? 0 : guarded_channel_word(DVM_R.m);
}

"""


def adapt(source):
    """Preserve legacy entry points and guard against changes to their bodies."""
    for index, (pattern, digest, dispatch) in enumerate(CHANNELS):
        match = re.search(pattern, source, re.S)
        if not match or hashlib.sha256(match.group().encode()).hexdigest() != digest:
            raise ValueError("channel constructor drift: " + pattern)
        body = match.group()
        offset = body.index("\n{") + 2
        selection = "\n    if (guarded_channel_selected())\n    {\n        " + dispatch + "\n    }\n"
        helpers = HELPERS if index == 0 else ""
        source = (source[:match.start()] + helpers + body[:offset] + selection
                  + body[offset:] + source[match.end():])
    return source
