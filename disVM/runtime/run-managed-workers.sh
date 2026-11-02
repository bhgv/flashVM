#!/usr/bin/env sh
# Run the packaged channel/root example with TWO actual managed OS executors.
# Arguments select JIT, quantum or diagnostics; native GC remains required.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$root/disvm" --gc-worker --managed-workers 2 --stats "$@" "$root/managed-workers.dis"
