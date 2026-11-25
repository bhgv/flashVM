#!/bin/sh
# Copyright (c) 2026 bhgv
# EN: Run the embedded renderer for legacy local replay and earlier fixtures.
# RU: Запустить встроенный рендерер для прежних локальных сценариев replay.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$SCRIPT_DIR/disvm-editor" "$@" "$SCRIPT_DIR/desktop-ide.dis"
