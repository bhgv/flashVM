#!/usr/bin/env python3
"""Changing only VERSION.txt must refresh project metadata during an ordinary build."""
import argparse
import pathlib
import shutil
import subprocess
import tempfile
import time


def run(command, cwd=None):
    """Expose configure/build diagnostics only if the actual CMake process fails."""
    completed = subprocess.run(command, capture_output=True, text=True, timeout=25, cwd=cwd)
    if completed.returncode:
        raise RuntimeError(completed.stdout + completed.stderr)


def main():
    """Use the production version module in an isolated compiler-free native project."""
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--module", required=True)
    parser.add_argument("--generator")
    parser.add_argument("--make-program")
    parser.add_argument("--platform")
    parser.add_argument("--toolset")
    args = parser.parse_args()
    cmake = str(pathlib.Path(shutil.which(args.cmake) or args.cmake).resolve())
    module = pathlib.Path(args.module).resolve().as_posix().replace('"', '\\"')
    with tempfile.TemporaryDirectory(prefix="flashvm-version-") as directory:
        source = pathlib.Path(directory)
        build = source / "build"
        version = source / "VERSION.txt"
        version.write_text("1.2.3\n", encoding="utf-8")
        (source / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.10)\n"
            f'include("{module}")\n'
            'project(VersionRefresh VERSION "${FVM_VERSION}" LANGUAGES NONE)\n'
            'file(WRITE "${CMAKE_BINARY_DIR}/observed-version.txt" "${PROJECT_VERSION}\\n")\n'
            "add_custom_target(version_probe)\n",
            encoding="utf-8",
        )
        build.mkdir()
        command = [cmake]
        if args.generator:
            command += ["-G", args.generator]
        if args.make_program:
            command += ["-DCMAKE_MAKE_PROGRAM=" + args.make_program]
        if args.platform:
            command += ["-A", args.platform]
        if args.toolset:
            command += ["-T", args.toolset]
        command.append(str(source))
        run(command, cwd=build)
        ordinary_build = [
            cmake,
            "--build",
            str(build),
            "--config",
            "Release",
            "--target",
            "version_probe",
        ]
        run(ordinary_build)
        observed = build / "observed-version.txt"
        if observed.read_text(encoding="utf-8").strip() != "1.2.3":
            raise RuntimeError("Initial project version does not match VERSION.txt")
        # Cross timestamp resolution of Make and filesystems without future-dating inputs.
        time.sleep(1.1)
        version.write_text("1.2.4\n", encoding="utf-8")
        run(ordinary_build)
        if observed.read_text(encoding="utf-8").strip() != "1.2.4":
            raise RuntimeError("Version-only edit did not reconfigure project metadata")
        run(ordinary_build)
        if observed.read_text(encoding="utf-8").strip() != "1.2.4":
            raise RuntimeError("Repeated build lost refreshed version metadata")
    print("PASS version-only edit refreshes CMake metadata through an ordinary build")


if __name__ == "__main__":
    main()
