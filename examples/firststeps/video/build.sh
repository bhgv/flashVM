#!/usr/bin/env bash
# Reproduce real SWF6 video fixtures from synthetic, redistributable content.
set -euo pipefail # Preserve encoder failure instead of packaging an empty movie.
HERE=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) # Resolve this example directory.
ENCODER=${FFMPEG:-ffmpeg} # Explicit FFmpeg executable; not needed to run ordinary workspace builds.
"$ENCODER" -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=160x120:rate=12' -t 2 -an -c:v flv -pix_fmt yuv420p -f swf -y "$HERE/video-only.swf" # Encode 24 Sorenson Spark frames into embedded SWF video tags.
"$ENCODER" -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=160x120:rate=12' -f lavfi -i 'sine=frequency=440:sample_rate=22050' -t 2 -c:v flv -pix_fmt yuv420p -c:a libmp3lame -ar 22050 -ac 1 -b:a 32k -f swf -y "$HERE/video-audio.swf" # Add a synthetic 440 Hz mono MP3 stream to the same video scene.
