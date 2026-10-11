#!/usr/bin/env python3
"""Exercise the DEFS.txt contract with real CMake configurations, offline."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from build_definitions import read_keys

parser = argparse.ArgumentParser()
parser.add_argument('--cmake', default='cmake')
parser.add_argument('--report', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
base = (root / 'DEFS.txt').read_text()
results = []


def select(text, old, new):
    return text.replace('\n' + old + '\n', '\n# ' + old + '\n').replace(
        '\n# ' + new + '\n', '\n' + new + '\n')


cases = [
    ('default', base, None),
    ('cairo', select(base, 'GRAPHICS_AGG', 'GRAPHICS_CAIRO'), None),
    ('no_graphics', select(base, 'GRAPHICS_AGG', 'NO_GRAPHICS'), None),
    ('no_fonts', select(base, 'FONTS', 'NO_FONTS'), None),
    ('missing_fonts', base.replace('\nFONTS\n', '\n# FONTS\n'), 'requires exactly one key'),
    ('conflicting_fonts', base.replace('# NO_FONTS\n', 'NO_FONTS\n'), 'requires exactly one key'),
    ('no_codecs', select(base, 'IMAGE_CODECS', 'NO_IMAGE_CODECS'), None),
    ('missing_codecs', base.replace('\nIMAGE_CODECS\n', '\n# IMAGE_CODECS\n'), 'requires exactly one key'),
    ('conflicting_codecs', base.replace('# NO_IMAGE_CODECS\n', 'NO_IMAGE_CODECS\n'), 'requires exactly one key'),
    ('explicit_linux', select(base, 'AUTO_PLATFORM', 'LINUX'), None),
    ('duplicate', base + '\nSQLITE\n', 'Duplicate DEFS.txt key'),
    ('unknown', base + '\nUNKNOWN_BACKEND\n', 'Unknown DEFS.txt key'),
    ('missing_graphics', base.replace('\nGRAPHICS_AGG\n', '\n# GRAPHICS_AGG\n'), 'requires exactly one key'),
    ('conflicting_graphics', base.replace('# GRAPHICS_CAIRO\n', 'GRAPHICS_CAIRO\n'), 'requires exactly one key'),
    ('wrong_platform', select(base, 'AUTO_PLATFORM', 'WINDOWS'), 'conflicts with target'),
    ('jit', select(base, 'INTERPRETER', 'JIT'), 'not implemented'),
    ('media', select(base, 'NO_MEDIA', 'MEDIA'), 'not implemented'),
]
for output in ('FB', 'GLES', 'OGL', 'DX', 'SDL1', 'SDL2', 'SDL3'):
    cases.append((output.lower(), select(base, 'HEADLESS', output), 'not implemented'))

temporary_root = root / 'build-definitions'
temporary_root.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='flashvm-definitions-', dir=temporary_root) as directory:
    directory = Path(directory)
    for name, text, diagnostic in cases:
        definitions = directory / (name + '.txt')
        definitions.write_text(text)
        command = [args.cmake, '-S', str(root), '-B', str(directory / name),
                   '-DFVM_DEFS_FILE=' + str(definitions),
                   '-DFVM_BUILD_TESTS=OFF', '-DFVM_BUILD_EXAMPLES=OFF', '-DBUILD_SHARED_LIBS=ON']
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=60)
        if diagnostic:
            assert result.returncode != 0 and diagnostic in result.stdout, result.stdout
            try:
                read_keys(definitions)
            except ValueError:
                pass
            else:
                raise AssertionError('Standalone reader accepted ' + name)
        else:
            assert result.returncode == 0, result.stdout
            assert read_keys(definitions)
        results.append({'case': name, 'passed': True})
        print('PASS', name, flush=True)
    # Cache switches are a compatibility check, not an override of the file.
    conflict = subprocess.run([args.cmake, '-S', str(root), '-B', str(directory / 'cache'),
                              '-DFVM_ENABLE_SQLITE=OFF'], text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    assert conflict.returncode != 0 and 'conflicts with DEFS.txt' in conflict.stdout, conflict.stdout
    results.append({'case': 'cache_conflict', 'passed': True})
    # File changes trigger reconfiguration on a regular build, without -S/-B.
    definitions = directory / 'reconfigure.txt'
    definitions.write_text(base)
    build = directory / 'reconfigure'
    subprocess.run([args.cmake, '-S', str(root), '-B', str(build),
                    '-DFVM_DEFS_FILE=' + str(definitions),
                    '-DFVM_BUILD_TESTS=OFF', '-DFVM_BUILD_EXAMPLES=OFF', '-DBUILD_SHARED_LIBS=ON'],
                   check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    definitions.write_text(base + '\nGRAPHICS_CAIRO\n')
    result = subprocess.run([args.cmake, '--build', str(build)], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
    assert result.returncode != 0 and 'requires exactly one key' in result.stdout, result.stdout
    results.append({'case': 'automatic_reconfigure', 'passed': True})
if args.report:
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'status': 'passed', 'count': len(results),
                                      'platform': 'Linux', 'cases': results}, indent=2) + '\n')
print('PASS:', len(results), 'definition cases')
