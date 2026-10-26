#!/usr/bin/env sh
# Resolve installed modules beside this launcher while preserving the document cwd.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -n "${FVM_EDITOR_FONT:-}" ]; then
    set -- --font "$FVM_EDITOR_FONT" "$@"
fi
exec "$root/disvm-editor" --stats "$@" "$root/ide.dis"
