#!/bin/sh
# Run the actual Limbo/Coco/R catalog example; optional runtime flags remain literal argv.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/disvm" "$@" tokenquery-demo.dis
