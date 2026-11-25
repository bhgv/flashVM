#!/bin/sh
# Copyright (c) 2026 bhgv
# EN: Run the installed Limbo/AS desktop over one private StyxRPC socketpair.
# RU: Запуск установленной IDE на Limbo/AS по одной локальной паре StyxRPC.
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mode=--no-jit
document=
document_mode=--document
capture=
window=--window
while [ "$#" -gt 0 ]; do
    case "$1" in
        --jit|--no-jit) mode=$1; shift ;;
        --document|--new) document_mode=$1; document=${2:?missing document path}; shift 2 ;;
        --capture) capture=${2:?missing capture path}; shift 2 ;;
        --headless) window=; shift ;;
        --help)
            printf '%s\n' 'Usage: run-desktop-ide.sh [--document PATH | --new PATH] [--jit|--no-jit] [--headless] [--capture PPM]'
            exit 0 ;;
        *) printf 'Unknown paired desktop option: %s\n' "$1" >&2; exit 2 ;;
    esac
done
if [ -n "$capture" ]; then
    case "$capture" in /*) ;; *) capture=$PWD/$capture ;; esac
    set -- --capture "$capture"
fi
if [ -n "$document" ]; then
    case "$document" in
        /*) ;;
        *) document=$PWD/$document ;;
    esac
    workspace=$(CDPATH= cd -- "$(dirname -- "$document")" && pwd -P)
    document=/$(basename -- "$document")
    cd -- "$workspace"
fi
exec "$SCRIPT_DIR/disvm" --styx-local-pair "$SCRIPT_DIR/flashvm_styx_player" "$mode" ${window:+"$window"} "$@" -- \
    --at /rpc --assets "$SCRIPT_DIR" "$document_mode" "$document" -- --native-spawn "$mode" "$SCRIPT_DIR/desktop-paired.dis"
