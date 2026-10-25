#!/usr/bin/env sh
# Run the installed real windowed player, independently of the caller's current directory.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Optional arguments are player options; the default movie exercises graphics/input/audio.
exec "$root/flashvm_player" "$@" "$root/examples/player/animation_audio/02-input-audio.swf"
