#!/usr/bin/env sh
# Exercise the packaged retained libraries on the actual native Dis owner.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$root/disvm" --stats "$@" "$root/libraries-check.dis"
