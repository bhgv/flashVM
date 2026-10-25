#!/usr/bin/env python3
"""Package already built ARM64 player/SDL libraries with SDK Java, test movies and a test signature."""
import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import zipfile


def run(arguments, environment):
    """Stop on the exact failed SDK operation; never publish a partly signed APK."""
    subprocess.run(list(map(str, arguments)), check=True, env=environment)


def arm64_library(path):
    """Reject another ABI or ELF segments lacking Android's 16 KiB page alignment."""
    data = path.read_bytes()
    if data[:5] != b"\x7fELF\x02" or data[5] != 1 or struct.unpack_from("<H", data, 18)[0] != 183:
        raise ValueError(str(path) + ": expected little-endian AArch64 ELF64")
    phoff = struct.unpack_from("<Q", data, 32)[0]
    phsize, count = struct.unpack_from("<HH", data, 54)
    for index in range(count):
        offset = phoff + index * phsize
        if struct.unpack_from("<I", data, offset)[0] == 1:
            if struct.unpack_from("<Q", data, offset + 48)[0] < 16384:
                raise ValueError(str(path) + ": PT_LOAD alignment is below 16 KiB")


def main():
    """Use explicit local SDK paths; no Gradle, dependency downloads or private signing credentials."""
    parser = argparse.ArgumentParser(description=__doc__)
    for name in (
        "native",
        "sdl-lib",
        "sdl-source",
        "android-jar",
        "build-tools",
        "java-home",
        "work",
        "output",
    ):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--strip", type=Path, help="Optional NDK llvm-strip for copied libraries")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    component = root.parent.parent
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ, JAVA_HOME=str(args.java_home.resolve()))
    # Rebuild classes/assets rather than accidentally including remnants of another application.
    for name in ("classes", "dex", "assets"):
        folder = work / name
        if folder.exists():
            shutil.rmtree(folder)
        folder.mkdir()
    sources = list((args.sdl_source / "android-project/app/src/main/java").rglob("*.java"))
    sources += list((root / "java").rglob("*.java"))
    run(
        [
            args.java_home / "bin/javac",
            "-encoding",
            "UTF-8",
            "-source",
            "8",
            "-target",
            "8",
            "-classpath",
            args.android_jar,
            "-d",
            work / "classes",
        ]
        + sources,
        environment,
    )
    classes = list((work / "classes").rglob("*.class"))
    run(
        [
            args.build_tools / "d8",
            "--lib",
            args.android_jar,
            "--min-api",
            "23",
            "--output",
            work / "dex",
        ]
        + classes,
        environment,
    )
    for movie in (component / "examples/player/animation_audio").glob("*.swf"):
        shutil.copy2(movie, work / "assets" / movie.name)
    licenses = work / "assets/licenses"
    licenses.mkdir()
    license_file = (
        component / "LICENSE" if (component / "LICENSE").exists() else component.parent / "LICENSE"
    )
    for source, name in (
        (license_file, "FlashVM-MIT.txt"),
        (component / "external/agg/upstream/copying", "AGG.txt"),
        (component / "external/zlib/upstream/LICENSE", "zlib.txt"),
        (args.sdl_source / "LICENSE.txt", "SDL2.txt"),
    ):
        shutil.copy2(source, licenses / name)
    # Static libc++ and NDK runtime notices accompany the copied native code.
    if args.strip:
        notice = args.strip.resolve().parent.parent / "NOTICE"
        if not notice.is_file():
            raise ValueError("Missing NDK toolchain NOTICE beside llvm-strip")
        shutil.copy2(notice, licenses / "Android-toolchain.txt")
    unsigned, aligned = work / "unsigned.apk", work / "aligned.apk"
    version = (component / "VERSION.txt").read_text().strip()
    version_code = int(version.split(".")[1])
    run(
        [
            args.build_tools / "aapt2",
            "link",
            "-I",
            args.android_jar,
            "--manifest",
            root / "AndroidManifest.xml",
            "--min-sdk-version",
            "23",
            "--target-sdk-version",
            "35",
            "--version-code",
            version_code,
            "--version-name",
            version,
            "-A",
            work / "assets",
            "-o",
            unsigned,
        ],
        environment,
    )
    with zipfile.ZipFile(unsigned, "a") as archive:
        archive.write(work / "dex/classes.dex", "classes.dex", compress_type=zipfile.ZIP_DEFLATED)
        for name, source in (("libflashvm_android.so", args.native), ("libSDL2.so", args.sdl_lib)):
            copied = work / name
            shutil.copy2(source, copied)
            if args.strip:
                run([args.strip, "--strip-unneeded", copied], environment)
            arm64_library(copied)
            archive.write(copied, "lib/arm64-v8a/" + name, compress_type=zipfile.ZIP_STORED)
    run([args.build_tools / "zipalign", "-P", "16", "-f", "4", unsigned, aligned], environment)
    key = work / "test.keystore"
    if not key.exists():
        run(
            [
                args.java_home / "bin/keytool",
                "-genkeypair",
                "-keystore",
                key,
                "-storepass",
                "android",
                "-alias",
                "flashvm-test",
                "-keypass",
                "android",
                "-dname",
                "CN=bhgv FlashVM Test",
                "-keyalg",
                "RSA",
                "-keysize",
                "2048",
                "-validity",
                "3650",
            ],
            environment,
        )
    # This disposable test identity is deliberately not a production release signing key.
    run(
        [
            args.build_tools / "apksigner",
            "sign",
            "--ks",
            key,
            "--ks-pass",
            "pass:android",
            "--key-pass",
            "pass:android",
            "--v4-signing-enabled",
            "false",
            "--out",
            args.output,
            aligned,
        ],
        environment,
    )
    run([args.build_tools / "apksigner", "verify", "--verbose", args.output], environment)
    run([args.build_tools / "zipalign", "-c", "-P", "16", "4", args.output], environment)
    print(args.output)


if __name__ == "__main__":
    main()
