#!/usr/bin/env sh
# Check installed executables without a compiler checkout or reference emu.
# Arguments: optional --window followed by player options. Returns first failure.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "${1:-}" = --window ]; then
    shift
    exec "$root/flashvm/run-demo.sh" "$@"
fi
if [ "$#" -ne 0 ]; then
    printf '%s\n' 'usage: run-checks.sh [--window [player options]]' >&2
    exit 2
fi
checks=0
for package in "$root"/flashvm/*; do
    if [ -x "$package/flashvm-console" ]; then
        actual=$("$package/flashvm-console" "$package/arithmetic.swf")
        if [ "$actual" != 42 ]; then
            printf 'FAIL arithmetic in %s: %s\n' "$package" "$actual" >&2
            exit 1
        fi
        printf 'PASS installed FlashVM arithmetic: %s\n' "$package"
        checks=$((checks + 1))
    fi
done
for component in disvm disvm-host; do
    if [ -x "$root/$component/disvm-check" ]; then
        "$root/$component/disvm-check" --all
        checks=$((checks + 1))
    fi
done
if [ -x "$root/disvm-runtime/disvm" ]; then
    sh "$root/disvm-runtime/run-console.sh"
    sh "$root/disvm-runtime/run-ide.sh"
    if [ -f "$root/disvm-runtime/run-modules.sh" ]; then
        sh "$root/disvm-runtime/run-modules.sh"
    fi
    checks=$((checks + 1))
fi
if [ "$checks" -eq 0 ]; then
    printf '%s\n' 'No runnable VM checks installed; build vm or disvm first.' >&2
    exit 1
fi
printf 'PASS: %s installed VM packages checked. Use --window for the graphical demo.\n' "$checks"
