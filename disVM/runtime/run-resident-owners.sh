#!/usr/bin/env sh
# Copyright (c) 2026 bhgv
# EN: Run the installed permanent-owner channel example; arguments are runtime options.
# RU: Запустить пример закреплённых владельцев и каналов; аргументы — опции runtime.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec "$root/disvm" --native-spawn --stats "$@" "$root/resident-owners.dis"
