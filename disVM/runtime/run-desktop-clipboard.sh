#!/bin/sh
# Run the session clipboard client with the matching installed native DisVM.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$SCRIPT_DIR/disvm" "$@" "$SCRIPT_DIR/desktop-clipboard.dis"
