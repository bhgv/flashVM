"""Exercise the real C89 tutorial, including exact native PCM bytes and rejected arguments."""
import os
import struct
import subprocess
import sys
import tempfile


def run(exe, *args):
    return subprocess.run([exe] + list(args), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, timeout=15)


def main():
    exe = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix="flashvm-mixer-") as root:
        target = os.path.join(root, "mixed pcm.pcm")
        for args in [("--demo",), ("--demo", target), ("--timeline-demo",), ("--timeline-demo", target)]:
            result = run(exe, *args)
            if result.returncode or b"first=-32768 tail=32512 active=0" not in result.stdout:
                raise RuntimeError((args, result.returncode, result.stdout, result.stderr))
        with open(target, "rb") as stream:
            if stream.read() != struct.pack("=4h", -32768, 32512, 0, 0):
                raise RuntimeError("actual native file differs from the independent PCM oracle")
        converted = os.path.join(root, "converted stereo.pcm")
        for args in [("--convert-demo",), ("--convert-demo", converted)]:
            result = run(exe, *args)
            if result.returncode or b"first=-32768 tail=27208 active=0" not in result.stdout:
                raise RuntimeError((args, result.returncode, result.stdout, result.stderr))
        with open(converted, "rb") as stream:
            if stream.read() != struct.pack("=8h", -32768, -32768, 27208, 27208, 32512, 32512, 0, 0):
                raise RuntimeError("converted stereo bytes differ from the independent PCM oracle")
        if run(exe, "--convert-demo", os.path.join(root, "missing", "out")).returncode != 1:
            raise RuntimeError("converted file-open failure was lost")
        if run(exe, "--demo", os.path.join(root, "missing", "out")).returncode != 1:
            raise RuntimeError("file-open failure was lost")
        if run(exe).returncode != 0:
            raise RuntimeError("usage should succeed without resources")
        for args in [("--bad",), ("--demo", target, "extra")]:
            if run(exe, *args).returncode != 2:
                raise RuntimeError("invalid CLI should fail before I/O")
    print("software mixer tutorial and exact file PCM passed")


if __name__ == "__main__":
    main()
