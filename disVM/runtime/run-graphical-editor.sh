#!/usr/bin/env sh
# Keep the caller's cwd for documents and resolve modules beside this launcher.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${FVM_EDITOR_FONT:-}" ]; then
    set -- --font "$FVM_EDITOR_FONT" "$@"
fi
exec "$root/disvm-editor" --stats "$@" "$root/graphical-editor.dis"
