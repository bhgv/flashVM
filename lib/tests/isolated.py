#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Build copied packages in isolation, relocate installed SDKs and link actual external C consumers.
# RU: Собрать копии пакетов изолированно, перенести SDK и слинковать настоящих внешних C-потребителей.
from pathlib import Path
import os
import shutil
import subprocess
import sys


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
    for package, option in [('styx', 'STYX_HOSTED'), ('libunibus', 'UNIBUS_QUEUES')]:
        source = area / (package + '-source')
        shutil.copytree(root / 'lib' / package, source, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
        for minimal in ([False] if sanitized else [False, True]):
            build = area / (package + ('-minimal' if minimal else '-hosted'))
            configure = [cmake, '-S', source, '-B', build, '-DBUILD_TESTING=ON', f'-D{option}={"OFF" if minimal else "ON"}']
            if sanitized:
                configure += ['-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -g', '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined']
                os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
                os.environ['UBSAN_OPTIONS'] = 'halt_on_error=1'
            run(configure, area, log)
            run([cmake, '--build', build, '-j4'], area, log)
            run([ctest, '--test-dir', build, '--output-on-failure'], area, log)
            if not minimal and not sanitized:
                prefix = area / (package + '-install')
                run([cmake, '--install', build, '--prefix', prefix], area, log)
                relocated = area / (package + '-relocated')
                prefix.rename(relocated)
                consumer = area / (package + '-consumer')
                consumer.mkdir()
                if package == 'styx':
                    find, target = 'Styx', 'Styx::codec'
                    code = '#include <styx/codec.h>\nint main(void) { Fcall f = {0}; f.type = Tversion; f.version = "9P2000"; f.msize = 8192; return sizeS2M(&f) == 0; }\n'
                else:
                    find, target = 'Unibus', 'Unibus::wire'
                    code = '#include <unibus/wire.h>\nint main(void) { unsigned char b[32]; size_t used; return fvm_srecord_write(1, "x", 1, b, sizeof(b), &used) != FVM_S_OK; }\n'
                (consumer / 'main.c').write_text(code)
                (consumer / 'CMakeLists.txt').write_text(f'cmake_minimum_required(VERSION 3.16)\nproject(Consumer C)\nfind_package({find} CONFIG REQUIRED)\nadd_executable(consumer main.c)\ntarget_link_libraries(consumer PRIVATE {target})\n')
                run([cmake, '-S', consumer, '-B', consumer / 'build', '-DCMAKE_PREFIX_PATH=' + str(relocated)], area, log)
                run([cmake, '--build', consumer / 'build'], area, log)
                run([consumer / 'build/consumer'], area, log)
                share = relocated / 'share' / package
                run([sys.executable, root / 'lib/tests/documentation.py', share], area, log)
            print('PASS', package, 'sanitized' if sanitized else 'minimal' if minimal else 'isolated hosted + relocated SDK', flush=True)
    if sanitized:
        run([sys.executable, root / 'lib/tests/interoperability.py', area / 'styx-hosted', area / 'libunibus-hosted'], area, log)
        print('PASS sanitized cross-library interoperability', flush=True)


if __name__ == '__main__':
    main()
