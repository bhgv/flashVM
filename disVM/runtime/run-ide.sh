#!/usr/bin/env sh
# Run actual IDE document/workspace services on the packaged native Dis runtime.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$root"
exec "$root/disvm" --stats "$@" ide-console.dis
