#!/usr/bin/env python3
"""Build a detached C89/C++98 fixture with the explicit Zig Linux toolchain.

This checks configuration rejection, archive creation and linked ELF identity.
It never executes a target program and is not a runtime/platform acceptance test.
All generated source and build products stay in the supplied empty work directory.
"""
import argparse
import os
import pathlib
import shutil
import struct
import subprocess

TARGETS = {'aarch64-linux-musl': 183, 'riscv64-linux-musl': 243,
           'x86_64-linux-musl': 62}


def run(command, log, expected=None):
    """Capture diagnostics and require either success or a specific rejection."""
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = result.stdout.decode('utf-8', 'replace')
    log.write_text(output, encoding='utf-8')
    if expected is None:
        if result.returncode:
            raise RuntimeError('command failed; see ' + str(log))
    elif not result.returncode or expected not in output:
        raise RuntimeError('missing expected rejection; see ' + str(log))


def check_elf(path, machine):
    """Require a little-endian ELF64 executable for the target, without an interpreter."""
    data = path.read_bytes()
    if len(data) < 64 or data[:7] != b'\x7fELF\x02\x01\x01':
        raise RuntimeError('not a little-endian ELF64 file: ' + str(path))
    kind, actual = struct.unpack_from('<HH', data, 16)
    if kind != 2 or actual != machine:
        raise RuntimeError('wrong executable type/CPU: ' + str(path))
    offset = struct.unpack_from('<Q', data, 32)[0]
    size, count = struct.unpack_from('<HH', data, 54)
    if size < 56 or not count or offset + size * count > len(data):
        raise RuntimeError('invalid program header table: ' + str(path))
    headers = [struct.unpack_from('<I', data, offset + i * size)[0] for i in range(count)]
    if 1 not in headers or 3 in headers:
        raise RuntimeError('missing load segment or unexpected dynamic interpreter: ' + str(path))


def main():
    """Exercise a toolchain copy outside the component and paths containing spaces."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', required=True, type=pathlib.Path)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--target', required=True, choices=sorted(TARGETS))
    parser.add_argument('--work', required=True, type=pathlib.Path)
    parser.add_argument('--artifact', action='append', default=[], type=pathlib.Path,
                        help='also inspect a linked FlashVM executable (repeatable)')
    args = parser.parse_args()
    work = args.work.resolve()
    if work.exists() and any(work.iterdir()):
        parser.error('--work must be absent or empty')
    work.mkdir(parents=True, exist_ok=True)
    source = work / 'detached source with spaces'
    source.mkdir()
    toolchain = source / 'toolchain copy.cmake'
    original = pathlib.Path(__file__).resolve().parents[1] / 'cmake/toolchains/zig-linux.cmake'
    shutil.copyfile(original, toolchain)
    (source / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.16)
project(toolchain_fixture C CXX)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-pedantic-errors)
add_library(fixture STATIC value.c)
add_executable(probe main.cpp)
target_link_libraries(probe PRIVATE fixture)
''')
    (source / 'value.c').write_text('int fixture_value(void) { return 7; }\n')
    (source / 'main.cpp').write_text('''extern "C" int fixture_value(void);
int main() { try { throw fixture_value(); } catch (int n) { return n == 7 ? 0 : 1; } }
''')
    zig = str(args.zig.resolve())
    build = work / 'build with spaces'
    base = [args.cmake, '-S', str(source), '-DCMAKE_TOOLCHAIN_FILE=' + str(toolchain)]
    settings = ['-DFVM_ZIG_EXECUTABLE=' + zig, '-DFVM_ZIG_TARGET=' + args.target]
    run(base + ['-B', str(build)] + settings, work / 'configure.log')
    run([args.cmake, '--build', str(build), '--parallel', '2'], work / 'build.log')
    check_elf(build / 'probe', TARGETS[args.target])
    if not (build / 'libfixture.a').read_bytes().startswith(b'!<arch>\n'):
        raise RuntimeError('missing conventional static archive')
    run(base + ['-B', str(build)] + settings, work / 'repeat.log')
    other = next(t for t in TARGETS if t != args.target)
    run(base + ['-B', str(build), '-DFVM_ZIG_EXECUTABLE=' + zig,
                '-DFVM_ZIG_TARGET=' + other], work / 'changed-target.log',
        'changed: use a fresh build directory')
    # Restore the target before separately testing a changed compiler installation.
    run(base + ['-B', str(build)] + settings, work / 'restore-target.log')
    compiler_alias = work / 'different zig installation'
    compiler_alias.symlink_to(args.zig.resolve())
    run(base + ['-B', str(build), '-DFVM_ZIG_EXECUTABLE=' + str(compiler_alias),
                '-DFVM_ZIG_TARGET=' + args.target], work / 'changed-compiler.log',
        'changed: use a fresh build directory')
    invalid = [([], 'FVM_ZIG_EXECUTABLE must name'),
               (['-DFVM_ZIG_EXECUTABLE=' + os.path.relpath(zig)],
                'FVM_ZIG_EXECUTABLE must name'),
               (['-DFVM_ZIG_EXECUTABLE=' + str(work)], 'FVM_ZIG_EXECUTABLE must name'),
               (['-DFVM_ZIG_EXECUTABLE=' + zig], 'FVM_ZIG_TARGET must be'),
               (['-DFVM_ZIG_EXECUTABLE=' + zig, '-DFVM_ZIG_TARGET=arm-linux-musl'],
                'FVM_ZIG_TARGET must be')]
    for index, (options, error) in enumerate(invalid):
        run(base + ['-B', str(work / ('invalid-' + str(index)))] + options,
            work / ('invalid-' + str(index) + '.log'), error)
    for artifact in args.artifact:
        check_elf(artifact, TARGETS[args.target])
    print('PASS: detached C89/C++98 archive and static ELF, repeat configure, '
          'target/compiler-change and five invalid-input rejections; target was not executed')


if __name__ == '__main__':
    main()
