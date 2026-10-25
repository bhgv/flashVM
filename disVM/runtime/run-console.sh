#!/usr/bin/env sh
# Run the packaged native Dis channel/Sys example using its matching ABI.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$root"
exec "$root/disvm" --stats "$@" console-check.dis
