#!/bin/sh
# Build the native POSIX Inferno mk with only a C compiler and ar, without CMake/make.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=${1:-"$root/../../build/mk"}
mkdir -p "$output"
output=$(CDPATH= cd -- "$output" && pwd)
cc=${CC:-cc}
ar=${AR:-ar}
mkdir -p "$output/objects"
objects=
# Each manifest-selected translation unit owns a unique path-derived object name.
while IFS= read -r source; do
    case "$source" in ''|'#'*) continue ;; esac
    name=$(printf '%s' "$source" | tr '/' '_')
    object="$output/objects/$name.o"
    "$cc" -std=gnu99 -O2 -fno-strict-aliasing ${CPPFLAGS:-} ${CFLAGS:-} \
        -I"$root/host/include" -I"$root/upstream/include" \
        -I"$root/upstream/utils/include" -I"$root/upstream/utils/mk" \
        -I"$root/upstream/utils/libregexp" -I"$root/upstream/Linux/arm/include" \
        -c "$root/upstream/$source" -o "$object"
    objects="$objects $name.o"
done < "$root/sources.list"
"$cc" -std=gnu99 -O2 -I"$root/host/include" -I"$root/upstream/include" \
    -c "$root/host/support.c" -o "$output/objects/host.o"
# Relative names are generated above from fixed slash-separated source paths.
# All output/source directories are quoted independently, including paths with spaces.
rm -f "$output/objects.a"
(cd "$output/objects" && "$ar" rcs ../objects.a $objects host.o)
"$cc" ${LDFLAGS:-} -Wl,-Map,"$output/link.map" -o "$output/mk" \
    "$output/objects.a" -lm
printf 'Built %s\n' "$output/mk"
