"""Generate guard-aware native error macros without modifying retained Proc ABI."""
import argparse
from pathlib import Path


# Replace exactly the selected macros. source is borrowed dat.h text; return
# transformed header or raise on upstream drift. setjmp stays in caller frame.
def error_header(source):
    old = '#define poperror()\tup->nerr--\n#define\twaserror()\t(up->nerr++, ossetjmp(up->estack[up->nerr-1]))'
    if source.count(old) != 1:
        raise ValueError('native error macro contract changed')
    new = '''/* Native checkpoints accompany existing Proc error labels; setjmp stays here. */
extern int dvm_native_error_push(void);
extern int dvm_native_error_pop(void);
#define poperror() do { if(!dvm_native_error_pop()) abort(); } while(0)
#define waserror() (dvm_native_error_push() ? ossetjmp(up->estack[up->nerr-1]) : (abort(), 0))'''
    return source.replace(old, new, 1)


# CLI owns only generated output. source/output are explicit paths; no source
# snapshots are written. Parse/read/drift failures propagate nonzero.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(error_header(args.source.read_text()))


if __name__ == '__main__':
    main()
