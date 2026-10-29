#!/bin/sh
# Run the portable pointer bytecode client using the matching installed DisVM.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$SCRIPT_DIR/disvm" "$@" "$SCRIPT_DIR/desktop-pointer.dis"
