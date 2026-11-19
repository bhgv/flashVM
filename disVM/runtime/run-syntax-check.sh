#!/bin/sh
# Copyright (c) 2026 bhgv
# EN: Run a compact generated-scanner example; quota stress tests stay separate.
# RU: Запустить короткий пример сканеров; стресс-проверки квот находятся отдельно.
# Optional arguments select --jit/--no-jit and runtime diagnostics, as separate argv.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"
exec "$SCRIPT_DIR/disvm" "$@" syntax-check.dis
