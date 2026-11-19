#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Build copied packages in isolation, relocate installed SDKs and link actual external C consumers.
# RU: Собрать копии пакетов изолированно, перенести SDK и слинковать настоящих внешних C-потребителей.
from pathlib import Path
import os
import shutil
import subprocess
import sys
from sdk import consumers


# EN: Execute borrowed argv in cwd, recording combined output to log; raise on any failed build/test.
# RU: Выполнить argv в cwd, записав общий вывод в log; отказ сборки/проверки вызывает исключение.
def run(argv, cwd, log):
    with log.open('a') as output:
        output.write('\n$ ' + ' '.join(map(str, argv)) + '\n')
        output.flush()
        subprocess.run(list(map(str, argv)), cwd=cwd, stdout=output, stderr=subprocess.STDOUT, check=True)


# EN: Own a fresh task workspace, copying only each package; cmake is an explicit CLI executable path.
# Optional --sanitize enables ASan/UBSan; leak scanning is disabled because this sandbox cannot run LSan.
# RU: Владеть новым task-каталогом, копировать только сам пакет; cmake — явный путь CLI.
# --sanitize включает ASan/UBSan; LSan отключён из-за ограничений sandbox.
def main():
    root = Path(__file__).resolve().parents[2]
    cmake = Path(sys.argv[1]).resolve()
    ctest = cmake.with_name('ctest')
    sanitized = '--sanitize' in sys.argv[2:]
    area = root / 'build' / ('standalone-isolated-sanitize' if sanitized else 'standalone-isolated')
    if area.exists():
        shutil.rmtree(area)
    area.mkdir()
    log = area / 'acceptance.log'
    run([sys.executable, root / 'lib/tests/c_only.py', cmake], area, log)
    for package, option in [('styx', 'STYX_HOSTED'), ('libunibus', 'UNIBUS_QUEUES')]:
        source = area / (package + '-source')
        shutil.copytree(root / 'lib' / package, source, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
        for minimal in ([False] if sanitized else [False, True]):
            build = area / (package + ('-minimal' if minimal else '-hosted'))
            configure = [cmake, '-S', source, '-B', build, '-DBUILD_TESTING=ON', f'-D{option}={"OFF" if minimal else "ON"}', '-DCMAKE_INSTALL_INCLUDEDIR=api/include', '-DCMAKE_CXX_COMPILER=/unavailable-cxx']
            if sanitized:
                configure += ['-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -g', '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined']
                os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
                os.environ['UBSAN_OPTIONS'] = 'halt_on_error=1'
            run(configure, area, log)
            run([cmake, '--build', build, '-j4'], area, log)
            run([ctest, '--test-dir', build, '--output-on-failure'], area, log)
            if not sanitized:
                label = package + ('-minimal' if minimal else '-hosted')
                prefix = area / (label + '-install')
                run([cmake, '--install', build, '--prefix', prefix], area, log)
                relocated = area / (label + '-relocated')
                prefix.rename(relocated)
                consumers(cmake, relocated, area / (label + '-consumer'), package, not minimal, log)
                run([sys.executable, root / 'lib/tests/documentation.py', relocated / 'share' / package], area, log)
            print('PASS', package, 'sanitized' if sanitized else 'minimal' if minimal else 'isolated hosted + relocated SDK', flush=True)
    if sanitized:
        run([sys.executable, root / 'lib/tests/interoperability.py', area / 'styx-hosted', area / 'libunibus-hosted'], area, log)
        print('PASS sanitized cross-library interoperability', flush=True)


if __name__ == '__main__':
    main()
