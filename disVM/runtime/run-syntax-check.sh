#!/bin/sh
# Run real Coco/R-generated scanners and the lazy Dis owner; no frame clock or Python.
# Optional arguments select --jit/--no-jit and runtime diagnostics, as separate argv.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/disvm" --test-control "$@" syntax-check.dis
