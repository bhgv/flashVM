#!/usr/bin/env python3
"""Resolve/build a pinned SDL2 SDK in the workspace; never install system packages."""

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tarfile
import urllib.request


MAX_DOWNLOAD = 16777216  # Maximum accepted X11 header-source archive bytes.


# Run literal argv in cwd, streaming build logs to stderr; return success as a boolean.
# No shell, credential lookup, global installation or administrator command is used.
def run(argv, cwd=None, required=True):
    result = subprocess.run(
        [str(value) for value in argv], cwd=cwd, stdout=sys.stderr, stderr=sys.stderr
    )
    if required and result.returncode:
        raise RuntimeError("dependency command failed: " + str(argv[0]))
    return result.returncode == 0


# Fetch exactly one public pinned Git commit into its cache; return the source directory.
# Existing sources must name that commit; normal builds never pull a moving branch.
def git_source(url, revision, destination, supplied=None):
    source = Path(supplied).resolve() if supplied else destination
    if not (source / ".git").exists():
        if supplied:
            raise RuntimeError("explicit SDL dependency source is not a Git checkout")
        source.mkdir(parents=True, exist_ok=True)
        run(["git", "init", "-q", source])
    current = subprocess.run(
        ["git", "-C", str(source), "rev-parse", "--verify", "HEAD"], capture_output=True, text=True
    )
    if current.returncode:
        if supplied:
            raise RuntimeError("explicit dependency source has no committed HEAD")
        run(["git", "-C", source, "fetch", "--depth", "1", url, revision])
        run(["git", "-C", source, "checkout", "--detach", "FETCH_HEAD"])
    actual = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    if actual != revision:
        raise RuntimeError("cached dependency revision mismatch: " + str(source))
    dirty = subprocess.check_output(
        ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=normal"], text=True
    )
    if dirty:
        raise RuntimeError("dependency source has local edits: " + str(source))
    return source


# Download a bounded HTTPS source archive and verify its pinned SHA-256 before use.
# cache is caller-owned build storage; return the verified archive path.
def archive(record, cache):
    cache.mkdir(parents=True, exist_ok=True)
    name = record["url"].rsplit("/", 1)[1]
    path = cache / name
    if path.exists() and hashlib.sha256(path.read_bytes()).hexdigest() == record["sha256"]:
        return path
    temporary = cache / (name + ".part-" + str(os.getpid()))
    try:
        print("Fetching build headers:", record["url"], file=sys.stderr)
        with urllib.request.urlopen(record["url"], timeout=30) as response, temporary.open(
            "wb"
        ) as output:
            if not response.geturl().startswith("https://"):
                raise RuntimeError("dependency redirect must remain HTTPS")
            total = 0
            while True:
                data = response.read(65536)
                if not data:
                    break
                total += len(data)
                if total > MAX_DOWNLOAD:
                    raise RuntimeError("dependency archive exceeded its byte limit")
                output.write(data)
        if hashlib.sha256(temporary.read_bytes()).hexdigest() != record["sha256"]:
            raise RuntimeError("dependency archive checksum mismatch: " + name)
        temporary.replace(path)
    finally:
        if temporary.exists():
            temporary.unlink()
    return path


# Extract only public header files and license texts; never execute archive content.
# record supplies root/hash; cache and prefix are owned dependency build directories.
def x11_headers(record, cache, prefix):
    path = archive(record, cache)
    with tarfile.open(path, "r:xz") as source:
        for item in source:
            parts = PurePosixPath(item.name).parts
            if not parts or parts[0] != record["root"] or ".." in parts or not item.isfile():
                continue
            relative = PurePosixPath(*parts[1:])
            name = str(relative)
            header = name.startswith("include/X11/") and (
                name.endswith(".h") or name.endswith("XlibConf.h.in")
            )
            license_file = len(relative.parts) == 1 and name.startswith(("COPYING", "LICENSE"))
            if not header and not license_file:
                continue
            if item.size > 1048576:
                raise RuntimeError("oversized header/license member")
            destination = (
                prefix / relative if header else prefix / "licenses" / record["root"] / relative
            )
            destination.parent.mkdir(parents=True, exist_ok=True)
            content = source.extractfile(item).read()
            if name.endswith("XlibConf.h.in"):
                destination = prefix / "include/X11/XlibConf.h"
                content = content.replace(b"#undef XTHREADS", b"#define XTHREADS 1")
                content = content.replace(b"#undef XUSE_MTSAFE_API", b"#define XUSE_MTSAFE_API 1")
            destination.write_bytes(content)


# Copy the ALSA public header closure; source is pinned, prefix is build-only SDK storage.
# Generate only configure's version/aggregate headers, preserving source licenses separately.
def alsa_headers(source, prefix, version):
    include = prefix / "include/alsa"
    include.mkdir(parents=True, exist_ok=True)
    for path in (source / "include").glob("*.h"):
        shutil.copy2(path, include / path.name)
    major, minor, patch = version.split(".")
    template = "/* Generated from pinned ALSA include/Makefile.am version rules. */\n"
    template += "#define SND_LIB_MAJOR " + major + "\n"
    template += "#define SND_LIB_MINOR " + minor + "\n"
    template += "#define SND_LIB_SUBMINOR " + patch + "\n"
    template += '#define SND_LIB_EXTRAVER ""\n'
    template += "#define SND_LIB_VER(maj,min,sub) (((maj)<<16)|((min)<<8)|(sub))\n"
    template += (
        "#define SND_LIB_VERSION SND_LIB_VER(SND_LIB_MAJOR,SND_LIB_MINOR,SND_LIB_SUBMINOR)\n"
    )
    template += '#define SND_LIB_VERSION_STR "' + version + '"\n'
    (include / "version.h").write_text(template)
    parts = [(source / "include/asoundlib-head.h").read_text()]
    parts.append("#include <endian.h>\n")
    for name in (
        "asoundef",
        "version",
        "global",
        "input",
        "output",
        "error",
        "conf",
        "pcm",
        "control",
        "mixer",
        "rawmidi",
        "ump",
        "timer",
        "hwdep",
        "seq_event",
        "seq",
        "seqmid",
        "seq_midi_event",
    ):
        parts.append("#include <alsa/" + name + ".h>\n")
    parts.append((source / "include/asoundlib-tail.h").read_text())
    (include / "asoundlib.h").write_text("\n".join(parts))
    licenses = prefix / "licenses/alsa-lib"
    licenses.mkdir(parents=True, exist_ok=True)
    for path in source.glob("COPYING*"):
        if path.is_file():
            shutil.copy2(path, licenses / path.name)


# Configure/build a tiny real header/library probe; return usable config path or None.
# cmake/probe/cache are explicit paths; override optionally selects the freshly built SDK.
def probe_sdk(cmake, probe, cache, override=None):
    cache.mkdir(parents=True, exist_ok=True)
    arguments = [cmake, probe]
    if override:
        arguments.append("-DSDL2_DIR=" + str(override))
    if not run(arguments, cwd=cache, required=False):
        return None
    result = (cache / "sdk-directory.txt").read_text().strip()
    if result == "MISSING" or not result:
        return None
    if not run(
        [cmake, "--build", cache, "--config", "Release", "--target", "sdk-probe"], required=False
    ):
        return None
    executable = cache / ("sdk-probe.exe" if os.name == "nt" else "sdk-probe")
    if not executable.exists():
        executable = cache / "Release" / executable.name
    capabilities = subprocess.check_output([str(executable)], text=True)
    (cache / "CAPABILITIES.txt").write_text(capabilities)
    print(capabilities, file=sys.stderr)
    return Path(result)


# Resolve a working system SDK or build the pinned fallback without sudo/package installation.
# CLI provides workspace/build root/compiler/jobs; stdout contains only the selected SDL2_DIR.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    root = args.workspace.resolve()
    cache = args.build_root.resolve() / "deps/sdl2"
    cache.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((root / "external/sdl2/bootstrap.json").read_text())
    probe = root / "tools/sdl2/probe"
    selected = probe_sdk(args.cmake, probe, cache / "system-probe")
    if selected:
        print(selected)
        return 0
    cached = cache / "sdk/lib/cmake/SDL2"
    marker = cache / "sdk/flashvm-bootstrap.json"
    if marker.is_file() and json.loads(marker.read_text()) == manifest:
        selected = probe_sdk(args.cmake, probe, cache / "built-probe", cached)
        if selected:
            print(selected)
            return 0
    print("Building pinned SDL2 locally; no system installation is required", file=sys.stderr)
    source = git_source(
        manifest["repository"],
        manifest["revision"],
        cache / ("source-" + manifest["revision"][:12]),
        os.environ.get("FVM_SDL2_SOURCE"),
    )
    build = cache / "build"
    prefix = cache / "sdk"
    build.mkdir(exist_ok=True)
    options = [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=" + str(prefix),
        "-DCMAKE_INSTALL_LIBDIR=lib",
        "-DSDL_SHARED=OFF",
        "-DSDL_STATIC=ON",
        "-DSDL_TEST=OFF",
        "-DSDL_TESTS=OFF",
        "-DSDL_OPENGL=OFF",
        "-DSDL_OPENGLES=OFF",
        "-DSDL_VULKAN=OFF",
    ]
    if sys.platform.startswith("linux"):
        libraries = (cache / "system-probe/native-libraries.txt").read_text().splitlines()
        headers = cache / "native-headers"
        archive_cache = Path(os.environ.get("FVM_X11_ARCHIVES", str(cache / "archives"))).resolve()
        if len(libraries) >= 2 and all(Path(value).is_file() for value in libraries[:2]):
            for record in manifest["headers"]:
                x11_headers(record, archive_cache, headers)
            options += [
                "-DX_INCLUDEDIR=" + str(headers / "include"),
                "-DX11_LIB=" + libraries[0],
                "-DXEXT_LIB=" + libraries[1],
                "-DSDL_X11=ON",
                "-DSDL_X11_SHARED=ON",
            ]
        if len(libraries) >= 3 and Path(libraries[2]).is_file():
            alsa = git_source(
                manifest["alsa_repository"],
                manifest["alsa_revision"],
                cache / ("alsa-source-" + manifest["alsa_revision"][:12]),
                os.environ.get("FVM_ALSA_SOURCE"),
            )
            alsa_headers(alsa, headers, manifest["alsa_version"])
            options += [
                "-DALSA_INCLUDE_DIR=" + str(headers / "include"),
                "-DALSA_LIBRARY=" + libraries[2],
                "-DASOUND_LIB=" + libraries[2],
                "-DSDL_ALSA=ON",
                "-DSDL_ALSA_SHARED=ON",
            ]
    run([args.cmake, source, *options], cwd=build)
    run(
        [
            args.cmake,
            "--build",
            build,
            "--config",
            "Release",
            "--target",
            "install",
            "--",
            "-j" + str(args.jobs),
        ]
    )
    selected = probe_sdk(args.cmake, probe, cache / "built-probe", prefix / "lib/cmake/SDL2")
    if not selected:
        raise RuntimeError("source-built SDL2 failed its real header/library probe")
    notice = prefix / "licenses"
    notice.mkdir(exist_ok=True)
    shutil.copy2(source / "LICENSE.txt", notice / "SDL2.txt")
    if (cache / "native-headers/licenses").exists():
        for directory in (cache / "native-headers/licenses").iterdir():
            destination = notice / directory.name
            destination.mkdir(exist_ok=True)
            for path in directory.iterdir():
                shutil.copy2(path, destination / path.name)
    shutil.copy2(cache / "built-probe/CAPABILITIES.txt", prefix / "CAPABILITIES.txt")
    (prefix / "flashvm-bootstrap.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(selected)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        print("SDL2 bootstrap:", error, file=sys.stderr)
        raise SystemExit(2)
