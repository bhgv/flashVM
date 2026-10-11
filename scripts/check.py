#!/usr/bin/env python3
"""Linux offline C89/C++98 compiler runner; DEFS.txt controls all build profiles."""
import argparse
import concurrent.futures
import json
import os
import pathlib
import subprocess
import sys

from build_definitions import cmake_sources, generated_headers, read_keys

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
parser.add_argument('--defs', type=pathlib.Path)
parser.add_argument('--limited-asan-stack', action='store_true',
                    help='GCC: disable stack instrumentation only in serialization exception frames (restricted /proc environments)')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
output = root / ('build-sanitize' if args.sanitize else 'build-check')
output.mkdir(exist_ok=True)
try:
    keys = read_keys(args.defs or root / 'DEFS.txt')
except ValueError as error:
    print(error, file=sys.stderr)
    sys.exit(1)
generated = output / 'generated'
generated_headers(root, generated, keys)
flags = ['-g', '-O1', '-Wall', '-Wextra', '-pedantic-errors', '-Iinclude',
         '-Iports', '-Iexternal/mini_regex', '-Iexternal/zlib/upstream',
         '-I' + str(generated), '-pthread']
flags += ['-D' + key + '=1' for key in keys]
if 'FONTS' in keys:
    flags += ['-DFVM_HAS_FONTS=1', '-DFRIBIDI_LIB_STATIC',
              '-Iexternal/freetype/upstream/include', '-Iexternal/harfbuzz/upstream/src',
              '-Iexternal/libunibreak/upstream/src', '-Iexternal/fribidi/upstream/lib',
              '-Iexternal/fribidi']
if 'GRAPHICS_AGG' in keys:
    flags += ['-Iexternal/agg/upstream/include', '-DFVM_HAS_AGG=1']
if 'GRAPHICS_CAIRO' in keys:
    flags += ['-DFVM_HAS_CAIRO=1']
if 'GRAPHICS_CAIRO' in keys or 'IMAGE_CODECS' in keys:
    for dependency, subdir in [('cairo', 'src'), ('pixman', 'pixman')]:
        flags += ['-Iexternal/' + dependency + '/upstream/' + subdir,
                  '-I' + str(generated / dependency)]
if 'IMAGE_CODECS' in keys:
    flags += ['-DFVM_HAS_IMAGE_CODECS=1', '-Iexternal/libpng/upstream',
              '-Iexternal/giflib/upstream', '-Iexternal/libjpeg/upstream',
              '-Iexternal/libjpeg', '-Iexternal/nanosvg/upstream']
if 'SQLITE' in keys:
    flags += ['-Iexternal/sqlite/upstream', '-Iexternal/sqlite', '-DFVM_HAS_SQLITE=1']
if args.sanitize:
    # The managed environment lacks /proc task access required by LeakSanitizer.
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
    os.environ['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer',
              '-fno-pie', '-no-pie']


def run(command):
    result = subprocess.run(command, cwd=root, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    # Fail on sanitizer diagnostics even if a runtime recovers with exit code 0.
    if result.returncode or (args.sanitize and
            any(marker in result.stdout for marker in ['runtime error:', 'ERROR: AddressSanitizer'])):
        raise RuntimeError(' '.join(map(str, command)) + '\n' + result.stdout)
    return result.stdout


sources = (list(root.glob('src/**/*.cpp')) + list(root.glob('src/**/*.c')) +
           [root / 'ports/file_port.c'] + list(root.glob('ports/linux/*.c')) + list(root.glob('external/mini_regex/*.c')))
sources += [root / 'external/zlib/upstream' / name for name in [
    'adler32.c', 'crc32.c', 'deflate.c', 'trees.c', 'inflate.c', 'inftrees.c',
    'inffast.c', 'zutil.c', 'compress.c', 'uncompr.c']]
if 'GRAPHICS_AGG' in keys:
    sources += list(root.glob('external/agg/upstream/src/*.cpp'))
    if 'FONTS' in keys:
        flags += ['-Iexternal/agg/upstream/font_freetype']
        sources += [root / 'external/agg/upstream/font_freetype/agg_font_freetype.cpp']
if 'GRAPHICS_CAIRO' in keys or 'IMAGE_CODECS' in keys:
    for dependency in ['cairo', 'pixman']:
        sources += cmake_sources(root / 'external' / dependency / 'CMakeLists.txt',
                                 'FVM_' + dependency.upper())
if 'IMAGE_CODECS' in keys:
    for dependency in ['libpng', 'giflib', 'libjpeg', 'nanosvg']:
        sources += cmake_sources(root / 'external' / dependency / 'CMakeLists.txt',
                                 'FVM_' + dependency.upper())
    sources += [root / 'external/cairo/upstream/src' / name for name in
                ['cairo-png.c', 'cairo-svg-surface.c']]
if 'FONTS' in keys:
    for dependency in ['freetype', 'harfbuzz', 'libunibreak', 'fribidi']:
        sources += cmake_sources(root / 'external' / dependency / 'CMakeLists.txt',
                                 'FVM_' + dependency.upper())
    if 'GRAPHICS_CAIRO' in keys or 'IMAGE_CODECS' in keys:
        sources += [root / 'external/cairo/upstream/src/cairo-ft-font.c']
if 'SQLITE' in keys:
    sources += [root / 'external/sqlite/upstream/sqlite3.c']
else:
    sources = [source for source in sources if source.name != 'sqlite_c.cpp']
if 'NO_OS_PORTS' in keys:
    sources = [source for source in sources if source.name not in ['input.c', 'network.c']]
if 'NO_RENDER_JOBS' in keys:
    sources = [source for source in sources if source.name != 'render_jobs.c']


def program_flags(source):
    # This test replaces new/delete and needs the historical UBSan-only exception.
    if args.sanitize and source.name == 'container_exceptions.cpp':
        return [flag.replace('-fsanitize=address,undefined', '-fsanitize=undefined')
                for flag in flags]
    selected = list(flags)
    if args.sanitize and args.limited_asan_stack and source.name == 'serialization_c.cpp':
        # libasan cannot unpoison unwound stack frames when /proc stack discovery
        # fails. Heap/global ASan and UBSan remain enabled in this translation unit.
        selected += ['--param=asan-stack=0']
    return selected


def compile_source(source):
    compile_flags = program_flags(source)
    if source.suffix == '.c':
        compile_flags += ['-Werror=implicit-function-declaration']
    if any('external/' + name + '/upstream/' in str(source) for name in ['cairo', 'pixman']):
        dependency = 'pixman' if 'external/pixman/upstream/' in str(source) else 'cairo'
        # config.h is private to each library; its directory must come first.
        compile_flags = ['-I' + str(generated / dependency)] + compile_flags
        compile_flags += ['-DHAVE_CONFIG_H=1', '-Iexternal/cairo/upstream']
    if 'external/freetype/upstream/' in str(source):
        compile_flags += ['-DFT2_BUILD_LIBRARY', '-DFT_CONFIG_OPTION_SYSTEM_ZLIB']
    if 'external/harfbuzz/upstream/' in str(source):
        compile_flags += ['-DHAVE_ATEXIT=1', '-DHAVE_PTHREAD=1', '-DHAVE_FREETYPE=1', '-DHAVE_OT=1', '-DHAVE_FALLBACK=1',
                          '-DHAVE_UCDN=1', '-DHB_NO_WIN1256=1',
                          '-Iexternal/harfbuzz/upstream/src/hb-ucdn', '-fno-strict-aliasing']
    if 'external/fribidi/upstream/' in str(source):
        compile_flags = ['-Iexternal/fribidi'] + compile_flags
        compile_flags += ['-DHAVE_CONFIG_H=1', '-DFRIBIDI_BUILD']
    if 'external/giflib/upstream/' in str(source):
        compile_flags += ['-D_POSIX_C_SOURCE=200809L']
    if 'external/libpng/upstream/' in str(source):
        compile_flags += ['-DPNG_ARM_NEON_OPT=0', '-DPNG_POWERPC_VSX_OPT=0',
                          '-DPNG_INTEL_SSE_OPT=0', '-DPNG_MIPS_MSA_OPT=0',
                          '-DPNG_LOONGARCH_LSX_OPT=0']
    if source.name == 'sqlite3.c':
        compile_flags += ['-DSQLITE_THREADSAFE=1', '-DSQLITE_OMIT_LOAD_EXTENSION=1',
                          '-DSQLITE_DQS=0', '-DSQLITE_DEFAULT_MEMSTATUS=0',
                          '-D_GNU_SOURCE=1', '-D_POSIX_C_SOURCE=200809L', '-D_DEFAULT_SOURCE=1']
    obj = output / (str(source.relative_to(root)).replace('/', '_') + '.o')
    compiler = ['g++', '-std=c++98'] if source.suffix in ['.cpp', '.cc'] else ['gcc', '-std=c89']
    run(compiler + compile_flags + ['-c', str(source), '-o', str(obj)])
    return str(obj)


try:
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        objects = list(pool.map(compile_source, sources))
    option_obj = None
    if args.sanitize:
        options = output / 'sanitizer_options.c'
        options.write_text('const char *__asan_default_options(void) {return "detect_leaks=0:halt_on_error=1";}\n'
                           'int __lsan_is_turned_off(void) {return 1;}\n')
        option_obj = output / 'sanitizer_options.o'
        run(['gcc', '-std=c89', '-c', str(options), '-o', str(option_obj)])
    archive = str(output / 'libflashvm.a')
    # ar would otherwise retain stale objects when switching profiles.
    pathlib.Path(archive).unlink(missing_ok=True)
    run(['ar', 'rcs', archive] + objects)
    programs = (sorted(root.glob('tests/*.c')) + sorted(root.glob('tests/*.cpp')) +
                sorted(root.glob('examples/**/main.c')) + sorted(root.glob('examples/**/main.cpp')))
    programs = [source for source in programs
                if ('FONTS' in keys or 'font_libraries' not in str(source))
                and ('IMAGE_CODECS' in keys or 'image_codecs' not in str(source))
                and ('SQLITE' in keys or 'sqlite' not in str(source))
                and ('OS_PORTS' in keys or 'os_ports' not in str(source))
                and ('RENDER_JOBS' in keys or not any(name in str(source)
                     for name in ['render_jobs', 'parallel_render']))]
    passed = []
    for source in programs:
        obj = compile_source(source)
        executable = output / (str(source.relative_to(root)).replace('/', '_') + '.test')
        run(['g++'] + program_flags(source) + [obj] +
            ([str(option_obj)] if option_obj else []) + [archive, '-lm', '-ldl', '-o', str(executable)])
        run(['timeout', '30', str(executable)])
        passed.append(str(source.relative_to(root)))
        print('PASS', source.relative_to(root), flush=True)
    if 'OS_PORTS' in keys:
        executable = output / 'tests_resource_network_process.c.test'
        run([sys.executable, str(root / 'scripts/test_resource_process.py'), str(executable)])
        passed.append('scripts/test_resource_process.py')
        print('PASS independent TCP owner/client processes', flush=True)
    report = {'status': 'passed', 'tests': passed, 'count': len(passed), 'flags': flags,
              'compiler': run(['gcc', '--version']).splitlines()[0], 'sanitizers': args.sanitize,
              'address_excluded_tests': ['tests/container_exceptions.cpp'] if args.sanitize else [],
              'leak_sanitizer': False if args.sanitize else None,
              'address_stack_excluded_sources': ['src/capi/serialization_c.cpp'] if args.sanitize and args.limited_asan_stack else [],
              'sqlite': 'bundled 3.50.4' if 'SQLITE' in keys else 'disabled',
              'definitions': keys, 'platform': 'Linux',
              'fonts': ['FreeType 2.14.3', 'HarfBuzz 1.8.8', 'libunibreak 6.1', 'FriBidi 1.0.16'] if 'FONTS' in keys else [],
              'image_codecs': ['libpng 1.6.59', 'giflib 5.2.2', 'libjpeg 9f', 'NanoSVG', 'Cairo PNG/SVG'] if 'IMAGE_CODECS' in keys else []}
    (output / 'test-results.json').write_text(json.dumps(report, indent=2) + '\n')
    suffix = '; ASan/UBSan (container exceptions: UBSan only; LeakSanitizer disabled: /proc unavailable)' if args.sanitize else ''
    if args.sanitize and args.limited_asan_stack:
        suffix += '; serialization stack ASan disabled explicitly; heap/global ASan and UBSan retained'
    print('PASS: %d tests and examples; strict C89/C++98%s' % (len(passed), suffix))
except Exception as error:
    print(error, file=sys.stderr)
    sys.exit(1)
