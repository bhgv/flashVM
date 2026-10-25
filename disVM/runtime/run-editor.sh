#!/usr/bin/env sh
# Run the interactive Limbo editor against actual Dis document/history services.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$root/disvm" --stats "$@" "$root/editor-console.dis"
