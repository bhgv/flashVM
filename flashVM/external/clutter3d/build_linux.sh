#!/bin/sh
# Build the selected fork's Cogl and Clutter as static Linux libraries, not the Inferno client.
# Usage: build_linux.sh /absolute/prepared-sources /absolute/sdk-prefix [jobs]
set -eu
sources=$1
prefix=$2
jobs=${3:-2}
case "$sources:$prefix" in
    /*:/*) ;;
    *) echo 'Both source and SDK prefix must be absolute paths' >&2; exit 1 ;;
esac
if [ ! -f "$sources/clutter3d-source.json" ] || [ -e "$sources/inferno-os" ]; then
    echo 'Use prepare_sources.py first; the Inferno client must be excluded' >&2
    exit 1
fi
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
    --enable-gl --enable-glx --disable-gles1 --disable-gles2
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
