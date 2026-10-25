#!/usr/bin/env sh
# Run actual native Dis/JIT diagnostic programs; no Python or external emu at runtime.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "$#" -gt 1 ]; then
    printf '%s\n' 'No module arguments accepted; use --help' >&2
    exit 2
fi
case "${1:---all}" in
    --help | -h)
        printf '%s\n' 'usage: disvm-check [--all|--word32|--word64]' \
            'Executes retained Dis interpreter/JIT diagnostics on native AMD64.' \
            'WORD32/WORD64 name operand models, not different host processes.' \
            'This diagnostic does not run arbitrary Limbo programs or the full IDE.'
        ;;
    --all)
        printf '%s\n' 'Dis/JIT diagnostic: native AMD64, WORD32 operand model'
        "$root/disvm-jit-word32"
        printf '%s\n' 'Dis/JIT diagnostic: native AMD64, WORD64 operand model'
        "$root/disvm-jit-word64"
        ;;
    --word32)
        exec "$root/disvm-jit-word32"
        ;;
    --word64)
        exec "$root/disvm-jit-word64"
        ;;
    *)
        printf '%s\n' 'Unknown diagnostic option; use --help' >&2
        exit 2
        ;;
esac
