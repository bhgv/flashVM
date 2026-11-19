# Copyright (c) 2026 bhgv
# EN: Prove standalone C guards accept C and reject C++ units/overrides before
# generation, without enabling a C++ compiler. Example/consumer languages remain free.
# RU: Проверить приём C и отказ C++ units/overrides до generation без включения
# C++ compiler. Языки примеров и потребителей остаются свободными.
from pathlib import Path
import subprocess
import sys
import tempfile


# EN: Configure one owned fixture in folder using borrowed guard/cmake executable.
# extra configures source/linker overrides; filename selects the unit suffix.
# Require the intended configure result, retaining output only for diagnostics.
# RU: Настроить fixture в folder с guard/cmake. extra задаёт source/linker override,
# filename — suffix unit. Требовать ожидаемый configure result, output хранить
# только для диагностики. C++ compiler намеренно не включается.
def fixture(cmake, folder, guard, filename, extra, success):
    folder.mkdir()
    (folder / filename).write_text('int library_value(void) { return 7; }\n')
    (folder / 'OnlyC.cmake').write_text(guard)
    (folder / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.16)\nproject(Guard LANGUAGES C)\n'
        f'add_library(example STATIC {filename})\n' + extra + '\n'
        'include(OnlyC.cmake)\nrequire_c_libraries(example)\n')
    result = subprocess.run([str(cmake), '-S', str(folder), '-B', str(folder / 'build'),
                             '-DCMAKE_CXX_COMPILER=/unavailable-cxx'], capture_output=True, text=True)
    output = result.stdout + result.stderr
    if (result.returncode == 0) != success or (not success and 'C-only library example:' not in output):
        raise RuntimeError(output)
    if success:
        subprocess.run([str(cmake), '--build', str(folder / 'build')], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


# EN/RU: Verify both independent copies, using only temporary fixtures and C.
# Проверить обе независимые копии только через временные fixtures и C.
def main():
    root = Path(__file__).resolve().parents[1]
    guards = [(root / package / 'cmake/OnlyC.cmake').read_text() for package in ('styx', 'libunibus')]
    if guards[0] != guards[1]:
        raise RuntimeError('standalone C-only contracts diverged')
    cmake = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='c-library-') as temporary:
        folder = Path(temporary)
        for index, (name, extra, success) in enumerate([
            ('unit.c', '', True), ('unit.cpp', '', False),
            ('unit.c', 'set_source_files_properties(unit.c PROPERTIES LANGUAGE CXX)', False),
            ('unit.c', 'set_target_properties(example PROPERTIES LINKER_LANGUAGE CXX)', False)
        ]):
            fixture(cmake, folder / str(index), guards[0], name, extra, success)
    print('PASS standalone libraries: C only; rejected C++ unit/source/linker overrides')


if __name__ == '__main__':
    main()
