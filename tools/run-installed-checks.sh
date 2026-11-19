#!/usr/bin/env sh
# Copyright (c) 2026 bhgv
# EN: Check installed console capabilities, including editor-only packages.
# RU: Проверка консольных возможностей, включая отдельный пакет editor.
# Arguments: optional --window followed by player options. Returns first failure.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ "${1:-}" = --window ]; then
    shift
    exec "$root/flashvm/run-demo.sh" "$@"
fi
if [ "$#" -ne 0 ]; then
    printf '%s\n' 'usage: run-checks.sh [--window [player options]]' >&2
    exit 2
fi
checks=0
for package in "$root"/flashvm/*; do
    if [ -x "$package/flashvm-console" ]; then
        actual=$("$package/flashvm-console" "$package/arithmetic.swf")
        if [ "$actual" != 42 ]; then
            printf 'FAIL arithmetic in %s: %s\n' "$package" "$actual" >&2
            exit 1
        fi
        printf 'PASS installed FlashVM arithmetic: %s\n' "$package"
        checks=$((checks + 1))
    fi
done
for component in disvm disvm-host; do
    if [ -x "$root/$component/disvm-check" ]; then
        "$root/$component/disvm-check" --all
        checks=$((checks + 1))
    fi
done
for component in disvm-runtime disvm-editor; do
    package="$root/$component"
    if [ -x "$package/disvm" ]; then
        sh "$package/run-console.sh"
        if [ -f "$package/run-ide-console.sh" ]; then
            sh "$package/run-ide-console.sh"
        else
            sh "$package/run-ide.sh"
        fi
        if [ -f "$package/run-modules.sh" ]; then
            sh "$package/run-modules.sh"
        fi
        if [ -f "$package/run-resident-owners.sh" ]; then
            sh "$package/run-resident-owners.sh"
        fi
        checks=$((checks + 1))
    fi
done
if [ "$checks" -eq 0 ]; then
    printf '%s\n' 'No runnable VM checks installed; build vm or disvm first.' >&2
    exit 1
fi
printf 'PASS: %s installed VM packages checked. Use --window for the graphical demo.\n' "$checks"
