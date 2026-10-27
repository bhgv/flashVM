"""Build a checked hosted view of the retained Inferno exception walker."""
import argparse
from pathlib import Path
import sys

sys.dont_write_bytecode = True  # Build adaptation must not leave caches in the source checkout.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "host"))
from prepare_native_registers import transform


# Replace one reviewed boundary, rejecting source drift before publishing output.
# All inputs are owned text; failures leave the prior generated product untouched.
def patch(source, before, after):
    if source.count(before) != 1:
        raise ValueError("exception host boundary changed: " + before)
    return source.replace(before, after, 1)


# CLI selects pinned source and generated output. Keep source bytes unchanged;
# adapt only TLS registers, permanent GC root registration and diagnostic services.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = transform(args.source.read_text())
    source = patch(source, '#include "raise.h"', '#include "raise.h"\n#include "exception_host.h"')
    source = patch(source, 'poolimmutable(D2H(exstr));',
                   'ptradd(D2H(exstr));\n\tSetmark(D2H(exstr)); /* [GC:] Permanent hosted fallback root. */')
    source = patch(source, 'kstrcpy(s->Sascii, p, s->max);', 'snprint(s->Sascii, s->max, "%s", p);')
    source = patch(source, 'modstatus(&DVM_R, name, sizeof(name))',
                   'dvm_exception_status(&DVM_R, name, sizeof(name))')
    args.output.write_text(source)


if __name__ == '__main__':
    main()
