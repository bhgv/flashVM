#!/usr/bin/env python3
"""Explicit, pinned Linux static SDK build; never invoked by CMake configuration."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request

VERSION = "6.1.2"
SHA256 = "3b624649725ecdc565c903ca6643d41f33bd49239922e45c9b1442c63dca4e38"
URL = "https://ffmpeg.org/releases/ffmpeg-" + VERSION + ".tar.xz"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", required=True, type=Path)
    parser.add_argument("--work", type=Path, default=Path(__file__).resolve().parent / "work")
    parser.add_argument("--archive", type=Path, help="Use an already downloaded official source archive")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if not os.sys.platform.startswith("linux"):
        parser.error("This recipe is Linux-native; supply a target SDK for other operating systems")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    archive = args.archive.resolve() if args.archive else work / ("ffmpeg-" + VERSION + ".tar.xz")
    if not archive.exists():
        if args.archive:
            parser.error("The supplied source archive does not exist")
        with urllib.request.urlopen(URL) as source, archive.open("wb") as dest:
            shutil.copyfileobj(source, dest)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("FFmpeg source SHA256 does not match the pinned release")
    source = work / ("ffmpeg-" + VERSION)
    if source.exists():
        shutil.rmtree(source)
    with tarfile.open(archive) as package:
        # The data filter rejects path traversal, special files and unsafe links/ownership.
        package.extractall(work, filter="data")
    build = work / "build"
    if build.exists():
        shutil.rmtree(build)
    build.mkdir()
    configure = [str(source / "configure"), "--prefix=" + str(args.prefix.resolve()),
                 "--disable-programs", "--disable-doc", "--disable-network",
                 "--disable-autodetect", "--disable-everything", "--disable-x86asm",
                 "--disable-avfilter", "--disable-postproc", "--enable-static", "--disable-shared",
                 "--enable-avdevice", "--enable-avformat", "--enable-avcodec",
                 "--enable-swresample", "--enable-swscale", "--enable-protocol=file",
                 "--enable-demuxer=wav,mp3,flv,mov,matroska,ogg,rawvideo,s16le,image2,nut",
                 "--enable-decoder=pcm_s16le,pcm_u8,mp3float,aac,flv,h263,vp6,vp6f,h264,rawvideo,bmp,ppm,png,vorbis",
                 "--enable-indev=v4l2"]
    subprocess.run(configure, cwd=build, check=True)
    subprocess.run(["make", "-j" + str(args.jobs)], cwd=build, check=True)
    subprocess.run(["make", "install"], cwd=build, check=True)
    print("Static FFmpeg SDK:", args.prefix.resolve())


if __name__ == "__main__":
    main()
