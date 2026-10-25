#!/usr/bin/env sh
# Select an actual installed GUI package; never substitute the action-only runner.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
for package in "$root/agg-sdl2-jit" "$root/agg-sdl2" "$root/cairo-sdl2-jit" "$root/cairo-sdl2"; do
    if [ -x "$package/run-demo.sh" ]; then
        exec "$package/run-demo.sh" "$@"
    fi
done
printf '%s\n' 'No windowed FlashVM package is installed in this output directory.' >&2
exit 1
