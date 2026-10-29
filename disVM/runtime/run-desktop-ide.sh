#!/bin/sh
# Run the shipped bytecode desktop; all view objects belong to FlashVM.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$SCRIPT_DIR/disvm-editor" "$@" "$SCRIPT_DIR/desktop-ide.dis"
