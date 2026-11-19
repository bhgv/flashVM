#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Exercise installed SDKs using independent C/C++98 consumers and real package-selection failures.
# RU: Проверять установленные SDK независимыми C/C++98 потребителями и отказами выбора пакета.
from pathlib import Path
import subprocess

PROBES = {  # Each fixture is compiled twice to test the same public declaration under C and C++98.
    'styx': ('styx_codec.c', 'styx_client.c'),
    'libunibus': ('unibus_wire.c', 'unibus_queues.c'),
}


# EN: Configure one independent discovery case in folder against prefix with supplied find arguments.
# cmake/log are borrowed paths. If reject is supplied, require failure containing that diagnostic;
# otherwise require success. Return the build path. No mutation of the installed SDK occurs.
# RU: Настроить отдельный discovery case в folder по prefix с аргументами find.
# cmake/log — заимствованные пути. При reject требовать отказ с этой диагностикой,
# иначе успех. Вернуть build path. Установленный SDK не изменяется.
def discover(cmake, prefix, folder, package, arguments, log, reject=None):
    folder.mkdir()
    (folder / 'CMakeLists.txt').write_text(
        '# Copyright (c) 2026 bhgv\n# EN/RU: Generated independent SDK discovery case / Независимая проверка SDK.\n'
        'cmake_minimum_required(VERSION 3.16)\nproject(Discover C)\n'
        f'find_package({package} {arguments})\n')
    build = folder / 'build'
    process = subprocess.run([str(cmake), '-S', str(folder), '-B', str(build), '-DCMAKE_PREFIX_PATH=' + str(prefix)],
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    with log.open('a') as output:
        output.write(f'\n$ discovery {package} {arguments}\n' + process.stdout)
    if reject is None:
        process.check_returncode()
    elif process.returncode == 0 or reject not in process.stdout:
        raise ValueError(f'expected SDK rejection containing {reject!r}: {process.stdout}')
    return build


# EN: Build/run real C and C++98 consumers from folder using relocated prefix.
# package is styx/libunibus; hosted selects available native targets. All paths are borrowed.
# Returns None; build/run/discovery failures propagate. No VM or original-source include is provided.
# RU: Собрать/выполнить C и C++98 consumers из folder по перенесённому prefix.
# package — styx/libunibus; hosted выбирает доступные native targets. Пути заимствованы.
# Возврата нет; ошибки передаются. VM и исходные include-пути не предоставляются.
def consumers(cmake, prefix, folder, package, hosted, log):
    name = 'Styx' if package == 'styx' else 'Unibus'
    wire = 'codec' if package == 'styx' else 'wire'
    native = 'client' if package == 'styx' else 'queues'
    required = wire + (' ' + native if hosted else '')
    if package == 'styx' and hosted:
        required += ' phy_server'
    build = discover(cmake, prefix, folder, name, '0.1.0 EXACT CONFIG REQUIRED COMPONENTS ' + required, log)
    cmake_source = (folder / 'CMakeLists.txt').read_text().replace('project(Discover C)', 'project(Consumer C CXX)')
    for profile, fixture in zip(('wire', 'native'), PROBES[package]):
        if profile == 'native' and not hosted:
            continue
        code = (Path(__file__).parent / 'consumers' / fixture).read_text()
        for extension in ('c', 'cpp'):
            target = profile + '_' + extension
            source = target + '.' + extension
            (folder / source).write_text(code)
            component = wire if profile == 'wire' else native
            cmake_source += f'add_executable({target} {source})\ntarget_link_libraries({target} PRIVATE {name}::{component})\n'
            cmake_source += f'set_target_properties({target} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)\n'
    sample = 'codec.c' if package == 'styx' else 'nested.c'
    (folder / 'documented.c').write_text((prefix / 'share' / package / 'examples' / sample).read_text())
    standard = 99 if package == 'styx' else 90
    cmake_source += f'add_executable(documented documented.c)\ntarget_link_libraries(documented PRIVATE {name}::{wire})\n'
    cmake_source += f'set_target_properties(documented PROPERTIES C_STANDARD {standard} C_STANDARD_REQUIRED YES C_EXTENSIONS NO)\n'
    if package == 'styx' and hosted:
        for extension in ('c', 'cpp'):
            code = (Path(__file__).parent / 'consumers/styx_phy.c').read_text()
            (folder / ('phy_' + extension + '.' + extension)).write_text(code)
            cmake_source += f'add_executable(phy_{extension} phy_{extension}.{extension})\ntarget_link_libraries(phy_{extension} PRIVATE Styx::phy_server)\n'
            cmake_source += f'set_target_properties(phy_{extension} PROPERTIES CXX_STANDARD 98 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)\n'
        carrier = prefix / 'share/styx/examples/memory_phy'
        sources = []
        for path in sorted(carrier.glob('*')):
            if path.suffix in ('.c', '.h'):
                if path.suffix == '.c':
                    sources.append(path.name)
        # Group private example headers/sources away from the SDK probes to avoid short-name collisions.
        memory = folder / 'memory_example'
        memory.mkdir()
        for path in sorted(carrier.glob('*')):
            if path.suffix in ('.c', '.h'):
                (memory / path.name).write_text(path.read_text())
        cmake_source += 'add_executable(memory_example ' + ' '.join('memory_example/' + name for name in sources) + ')\n'
        cmake_source += 'target_link_libraries(memory_example PRIVATE Styx::client Styx::phy_server)\nset_target_properties(memory_example PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)\n'
    (folder / 'CMakeLists.txt').write_text(cmake_source)
    with log.open('a') as output:
        for command in ([str(cmake), '-S', str(folder), '-B', str(build)], [str(cmake), '--build', str(build)]):
            subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, check=True)
        subprocess.run([str(build / 'documented')], stdout=output, stderr=subprocess.STDOUT, check=True)
        if package == 'styx' and hosted:
            for program in ('phy_c', 'phy_cpp', 'memory_example'):
                subprocess.run([str(build / program)], stdout=output, stderr=subprocess.STDOUT, check=True)
        for profile in ('wire', 'native') if hosted else ('wire',):
            for extension in ('c', 'cpp'):
                subprocess.run([str(build / (profile + '_' + extension))], stdout=output, stderr=subprocess.STDOUT, check=True)
    parent = folder.parent
    discover(cmake, prefix, parent / (folder.name + '-too-new'), name, '99.0 CONFIG REQUIRED', log, 'compatible')
    discover(cmake, prefix, parent / (folder.name + '-other-minor'), name, '0.0 CONFIG REQUIRED', log, 'compatible')
    discover(cmake, prefix, parent / (folder.name + '-unknown'), name, 'CONFIG REQUIRED COMPONENTS absent', log, 'component is unavailable')
    discover(cmake, prefix, parent / (folder.name + '-optional'), name, 'CONFIG REQUIRED OPTIONAL_COMPONENTS absent', log)
    discover(cmake, prefix, parent / (folder.name + '-optional-native'), name,
             'CONFIG REQUIRED OPTIONAL_COMPONENTS ' + native, log)
    if not hosted:
        discover(cmake, prefix, parent / (folder.name + '-missing-native'), name,
                 'CONFIG REQUIRED COMPONENTS ' + native, log, 'component is unavailable')
