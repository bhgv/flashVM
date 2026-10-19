#!/bin/sh
# Build the selected fork's Cogl and Clutter as static Linux libraries, not the Inferno client.
# Usage: build_linux.sh /absolute/prepared-sources /absolute/sdk-prefix [jobs] [glx|gles2]
set -eu
sources=$1
prefix=$2
jobs=${3:-2}
renderer=${4:-glx}
case "$renderer" in
    glx) set -- --enable-gl --enable-glx --disable-gles1 --disable-gles2 ;;
    gles2) set -- --disable-gl --disable-glx --disable-gles1 --enable-gles2 \
        --enable-xlib-egl-platform --disable-cogl-gles2 --with-default-driver=gles2 ;;
    *) echo 'Linux renderer must be glx or gles2' >&2; exit 1 ;;
esac
case "$sources:$prefix" in
    /*:/*) ;;
    *) echo 'Both source and SDK prefix must be absolute paths' >&2; exit 1 ;;
esac
if [ ! -f "$sources/clutter3d-source.json" ] || [ -e "$sources/inferno-os" ]; then
    echo 'Use prepare_sources.py first; the Inferno client must be excluded' >&2
    exit 1
fi
# Autotools can reuse old objects after flag changes: refuse a mixed renderer tree.
if [ -f "$sources/clutter3d-linux-build-profile.txt" ] &&
   [ "$(cat "$sources/clutter3d-linux-build-profile.txt")" != "$renderer" ]; then
    echo 'Prepare a new source directory when changing renderer' >&2
    exit 1
fi
if [ -f "$prefix/clutter3d-linux-renderer.txt" ] &&
   [ "$(cat "$prefix/clutter3d-linux-renderer.txt")" != "$renderer" ]; then
    echo 'Use a separate SDK prefix when changing renderer' >&2
    exit 1
fi
if [ "$renderer" != glx ] && [ -f "$prefix/clutter3d-source.json" ] &&
   [ ! -f "$prefix/clutter3d-linux-renderer.txt" ]; then
    echo 'Legacy SDK has no renderer marker; use a fresh prefix for GLES2' >&2
    exit 1
fi
printf '%s\n' "$renderer" > "$sources/clutter3d-linux-build-profile.txt"
# Explicit tools override the historical automake 1.11/1.12/1.13 probe on modern systems.
AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
export AUTOMAKE ACLOCAL
# Preserve caller-provided dependency SDK paths, then prefer the matching fork Cogl.
PKG_CONFIG_PATH="$prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export PKG_CONFIG_PATH
cd "$sources/sources/cogl-android"
NOCONFIGURE=1 ./autogen.sh
./configure --prefix="$prefix" --enable-static --disable-shared \
    --disable-introspection --disable-gtk-doc --disable-examples-install \
    "$@"
make -j"$jobs"
make install
cd "$sources/sources/clutter-android"
NOCONFIGURE=1 ./autogen.sh
./configure --prefix="$prefix" --enable-static --disable-shared \
    --disable-introspection --disable-gtk-doc --disable-tests --disable-examples \
    --enable-x11-backend --disable-gdk-backend --disable-android-backend
make -j"$jobs"
make install
# Only a successfully installed pair receives the source provenance marker.
cp "$sources/clutter3d-source.json" "$prefix/clutter3d-source.json"
printf '%s\n' "$renderer" > "$prefix/clutter3d-linux-renderer.txt"
