#!/bin/sh
# Copyright (c) 2026 bhgv
# EN: Build the paired sample with explicit project tools and a staged output directory.
# RU: Собрать парный пример указанными инструментами проекта с промежуточным каталогом.
set -eu

if [ "$#" -ne 7 ]; then
    echo "usage: $0 AS2_VIEWC FIR_BUILD_DIR SWL LIMBO_PARSE LIMBO_BOOTSTRAP DISVM_SOURCE OUTPUT_DIR" >&2
    exit 2
fi

as2=$1
fir=$2
swl=$3
limbo_parse=$4
limbo=$5
disvm=$6
output=$7
source=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$output"
stage=$(mktemp -d "$output/.terminal.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT
trap 'exit 1' HUP INT TERM

"$as2" "$source/terminal.as" "$stage/terminal.fir"
"$fir/fir1" check "$stage/terminal.fir"
"$fir/fir1_opt" "$stage/terminal.fir" "$stage/terminal.opt.fir"
"$fir/fir1" object "$stage/terminal.opt.fir" "$stage/terminal.swo"
"$swl" link "$stage/terminal.swf" "$stage/terminal.swo" --size 320 240 --fps 30 \
    --svf panel "$source/panel.svf" --place panel 20 30 1
"$limbo_parse" "$source/terminal.b"
# The bootstrap compiler records a path relative to its Inferno root.
# Bootstrap-компилятор записывает путь относительно корня Inferno.
EMU= ROOT="$source" "$limbo" -I "$disvm/upstream/module" -I "$disvm/module" \
    -o "$stage/terminal.dis" "$source/terminal.b"
mv -- "$stage/terminal.swf" "$stage/terminal.dis" "$output/"
