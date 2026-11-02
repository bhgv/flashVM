#!/usr/bin/env sh
# Run the packaged channel/root example; default automatic actual managed OS executors.
# Arguments select JIT, quantum or diagnostics; native GC remains required.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Select the elastic or fixed pool without duplicating the runtime's one-shot CLI option.
count=${DVM_MANAGED_WORKERS:-auto}
case "$count" in
    auto) ;;
    *[!0-9]*|'') echo 'DVM_MANAGED_WORKERS must be a positive count or auto' >&2; exit 2 ;;
    *) ;;
esac
exec "$root/disvm" --gc-worker --managed-workers "$count" --stats "$@" "$root/managed-workers.dis"
