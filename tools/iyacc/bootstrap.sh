#!/bin/sh
# Build Inferno iyacc using the explicitly selected native mk support archive.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=${1:-"$root/../../build/iyacc"}
mk_root=${MK_SOURCE_ROOT:-"$root/../mk"}
mk_support=${MK_SUPPORT_ROOT:-"$root/../../build/mk"}
mkdir -p "$output/objects" "$output/utils/lib"
output=$(CDPATH= cd -- "$output" && pwd)
mk_root=$(CDPATH= cd -- "$mk_root" && pwd)
mk_support=$(CDPATH= cd -- "$mk_support" && pwd)
cc=${CC:-cc}
# iyacc reads the parser skeleton from ROOT/utils/lib/yaccpar. Keep it beside
# the executable's build products, with an explicit overridable runtime ROOT.
cp "$root/upstream/utils/iyacc/yaccpar" "$output/utils/lib/yaccpar"
for source in utils/iyacc/yacc.c lib9/argv0.c lib9/print.c libbio/bputrune.c; do
    name=$(printf '%s' "$source" | tr '/' '_')
    "$cc" -std=gnu99 -O2 -fno-strict-aliasing ${CPPFLAGS:-} ${CFLAGS:-} \
        -I"$mk_root/host/include" -I"$mk_root/upstream/include" \
        "-DROOT=\"$output\"" -c "$root/upstream/$source" -o "$output/objects/$name.o"
done
"$cc" ${LDFLAGS:-} -o "$output/iyacc" \
    "$output/objects/utils_iyacc_yacc.c.o" "$output/objects/lib9_argv0.c.o" \
    "$output/objects/lib9_print.c.o" "$output/objects/libbio_bputrune.c.o" \
    "$mk_support/objects.a" -lm
printf 'Built %s\n' "$output/iyacc"
