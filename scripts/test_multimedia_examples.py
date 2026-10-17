#!/usr/bin/env python3
"""Exercise real file paths in multimedia tutorials; never open hardware."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
import wave


def run(program, *args, success=True):
    result = subprocess.run([str(program), *map(str, args)], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(f"{program}: {result.returncode}\n{result.stdout}\n{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--ffmpeg", action="store_true")
    args = parser.parse_args()
    build = args.build.resolve()
    suffix = ".exe" if (build / "flashvm_examples_multimedia_ffmpeg_decode_main.exe").exists() else ""
    def program(name):
        return build / ("flashvm_examples_multimedia_" + name + "_main" + suffix)
    with tempfile.TemporaryDirectory(prefix="flashvm-media-tutorials-") as folder:
        folder = Path(folder)
        raw = folder / "mono.raw"
        raw.write_bytes(struct.pack("=8h", 1, -2, 3, -4, 5, -6, 7, -8))
        if "Consumed 8 mono sample frames" not in run(program("custom_audio_bridge"), raw):
            raise RuntimeError("Custom PCM bridge did not consume the full real fixture")
        raw.write_bytes(b"\x01")
        run(program("custom_audio_bridge"), raw, success=False)
        ppm = folder / "output.ppm"
        run(program("custom_video_output"), ppm)
        if ppm.read_bytes() != b"P6\n2 1\n255\n\xff\0\0\0\xff\0":
            raise RuntimeError("Custom presenter wrote unexpected RGB bytes")
        if args.ffmpeg:
            wav = folder / "input.wav"
            with wave.open(str(wav), "wb") as output:
                output.setparams((1, 2, 8000, 0, "NONE", "not compressed"))
                output.writeframes(struct.pack("<8h", 1, -2, 3, -4, 5, -6, 7, -8))
            if "audio=8000Hz/1ch" not in run(program("ffmpeg_decode"), "audio", wav):
                raise RuntimeError("FFmpeg tutorial did not decode the WAV fixture")
            if "size=2x1" not in run(program("ffmpeg_decode"), "video", ppm):
                raise RuntimeError("FFmpeg tutorial did not decode the PPM fixture")
    print("PASS multimedia tutorials: real PCM source, malformed input, exact PPM output" +
          (", native FFmpeg WAV/PPM decoding" if args.ffmpeg else ""))


if __name__ == "__main__":
    main()
