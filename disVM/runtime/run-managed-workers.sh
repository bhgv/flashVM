#!/usr/bin/env sh
# Run the packaged channel/root example; default TWO actual managed OS executors.
# Arguments select JIT, quantum or diagnostics; native GC remains required.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Select the bounded pool without duplicating the runtime's one-shot CLI option.
count=${DVM_MANAGED_WORKERS:-2}
case "$count" in
    1|2|3|4|5|6|7|8) ;;
    *) echo 'DVM_MANAGED_WORKERS must be in 1..8' >&2; exit 2 ;;
esac
exec "$root/disvm" --gc-worker --managed-workers "$count" --stats "$@" "$root/managed-workers.dis"
