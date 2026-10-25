#!/usr/bin/env sh
# Run actual IDE document/workspace services on the packaged native Dis runtime.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$root/disvm" --stats "$@" "$root/ide-console.dis"
