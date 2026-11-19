# Copyright (c) 2026 bhgv
# EN: Maintain authored integration recipes for copied sources and relocatable C/C++ SDKs.
# RU: Хранить авторские рецепты интеграции копий исходников и переносимых C/C++ SDK.
from pathlib import Path
from .introductions import page


# EN: Write EN/RU integration guides into both packages below borrowed root (lib/).
# All commands describe current CMake exports; this generator is not a build dependency.
# RU: Записать EN/RU руководства в оба пакета под root (lib/).
# Команды описывают текущие CMake exports; генератор не является зависимостью сборки.
def main(root):
    for directory, package, option, component in [('styx', 'Styx', 'STYX_HOSTED', 'codec'), ('libunibus', 'Unibus', 'UNIBUS_QUEUES', 'wire')]:
        base = root / directory
        native = 'client' if directory == 'styx' else 'queues'
        optional = 'carrier' if directory == 'styx' else 'queues'
        for ru in (False, True):
            suffix = '.ru.md' if ru else '.md'
            text = f'# {directory}: ' + ('встраивание и установленный SDK' if ru else 'embedding and installed SDK') + '\n\nCopyright (c) 2026 bhgv\n\n'
            text += f'[README](../README{suffix}) · [Introduction](introduction{suffix}) · [Tutorial](tutorial{suffix}) · [API](index{suffix})\n\n'
            text += '## ' + ('Что выбрать' if ru else 'Choosing a route') + '\n\n'
            text += ('Установленный SDK отделяет сборку библиотеки от приложения. Копия каталога исходников позволяет менять адаптеры или выбирать MCU toolchain. Оба пути используют собственные исходники пакета: DisVM, FlashVM и соседняя библиотека не нужны. Нативная проверка выполнена на Linux amd64; это не приёмка всех физических устройств и ОС.\n\n' if ru else 'An installed SDK separates library builds from application builds. A copied source directory lets you customize adapters or select an MCU toolchain. Both routes use package-owned sources without DisVM, FlashVM or the sibling library. Native acceptance is Linux amd64, not every physical device or OS.\n\n')
            text += '## ' + ('1. Собрать и установить библиотеку' if ru else '1. Build and install the library') + f'''\n\n```sh
cmake -S . -B build-sdk -D{option}=OFF -DBUILD_TESTING=OFF
cmake --build build-sdk -j
cmake --install build-sdk --prefix "$PWD/sdk"
```

'''
            text += ('Команды выполняются из корня скопированного пакета. OFF выбирает минимальный профиль. Для нативного слоя используйте ON. BUILD_TESTING=OFF исключает учебные программы и их test-only зависимости. Для hosted Styx Python всё ещё нужен при выборе исходников lib9.\n\n' if ru else 'Run from the copied package root. OFF selects the minimal profile; use ON for native layers. BUILD_TESTING=OFF excludes teaching programs and their test-only dependencies. Hosted Styx still uses Python during lib9 source selection.\n\n')
            sample = 'codec.c' if directory == 'styx' else 'nested.c'
            standard = '99' if directory == 'styx' else '90'
            text += '## ' + ('2. Подключить SDK из отдельного приложения' if ru else '2. Consume from an independent application') + f'''\n\n```sh
mkdir ../device-example
cp examples/{sample} ../device-example/main.c
```

''' + ('Сохраните следующий текст как ../device-example/CMakeLists.txt. main.c — полный рабочий пример пакета, а не отсутствующая заготовка.\n\n' if ru else 'Save the following as ../device-example/CMakeLists.txt. main.c is a complete package example, not a missing placeholder.\n\n') + f'''```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
find_package({package} 0.1 CONFIG REQUIRED COMPONENTS {component})
add_executable(device main.c)
target_link_libraries(device PRIVATE {package}::{component})
set_target_properties(device PROPERTIES C_STANDARD {standard} C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
```

```sh
cmake -S ../device-example -B ../device-example/build -DCMAKE_PREFIX_PATH="$PWD/sdk"
cmake --build ../device-example/build
../device-example/build/device
```

'''
            text += ('CMAKE_PREFIX_PATH — корень установленного SDK, не include-каталог и не исходный репозиторий. После переноса всего SDK укажите новое место. Include-пути и транзитивные библиотеки задаёт target; не записывайте старый абсолютный путь вручную. Поддерживается CMAKE_INSTALL_INCLUDEDIR, в том числе api/include. Потребитель с C++ включает язык CXX в project и может выбрать CXX_STANDARD 98. C linkage клиента Styx сохраняет C ABI при вызове из C++.\n\n' if ru else 'CMAKE_PREFIX_PATH names the installed prefix, not its include directory or original checkout. After moving the whole SDK, point to its new location. Targets supply include paths and transitive libraries; do not hardcode old absolute paths. CMAKE_INSTALL_INCLUDEDIR is supported, including api/include. C++ applications enable CXX in project and may select CXX_STANDARD 98. Styx client C linkage preserves the C ABI for C++ calls.\n\n')
            text += '## ' + ('Версия и компоненты' if ru else 'Version and components') + '\n\n'
            text += ('Версия пакета — 0.1.0. Запрос 0.1 выбирает совместимую установленную версию той же minor-линии; минимальная запрошенная версия всё равно соблюдается. EXACT требует точного совпадения, иная minor/major-линия не принимается автоматически. Это правило обнаружения SDK, а не заявление о wire-версии или ABI всех будущих выпусков.\n\n' if ru else 'Package version is 0.1.0. Requesting 0.1 accepts a compatible installed version within that minor line while respecting the requested minimum. EXACT requires an exact version; another minor/major line is not automatically accepted. This is an SDK discovery policy, not a wire version or a promise about every future ABI.\n\n')
            text += '| ' + ('Компонент | Target | Доступность' if ru else 'Component | Target | Availability') + ' |\n| --- | --- | --- |\n'
            entries = [('codec','always / всегда'),('wire','always / всегда'), *[(v,'STYX_HOSTED=ON') for v in ('nine','server','carrier','walk','client','phy_server')]] if directory=='styx' else [('wire','always / всегда'),('queues','UNIBUS_QUEUES=ON')]
            for name, availability in entries:
                availability = availability.replace('always / всегда', 'всегда' if ru else 'always')
                text += f'| `{name}` | `{package}::{name}` | `{availability}` |\n'
            text += f'''\n```cmake
find_package({package} 0.1 CONFIG REQUIRED
    COMPONENTS {component} OPTIONAL_COMPONENTS {optional})
if({package}_{optional}_FOUND)
    target_link_libraries(device PRIVATE {package}::{optional})
endif()
```

'''
            text += ('Это отдельный вариант конфигурации вместо предыдущего find_package. REQUIRED-компонент, отсутствующий в сборке, останавливает конфигурацию с причиной. Отсутствующий OPTIONAL_COMPONENT не мешает использованию минимального слоя. Неизвестное имя также отклоняется, если оно REQUIRED.\n\n' if ru else 'This is an alternative to the earlier find_package call. A required component absent from the build stops configuration with a reason. An unavailable optional component leaves the minimal layer usable. Unknown required names are rejected too.\n\n')
            if directory=='styx':
                text += ('Для приложения с native client выбирайте Styx::client вместо Styx::codec: hosted lib9 уже содержит те же codec symbols. Не соединяйте эти две реализации в одном executable и не смешивайте их native заголовки. Styx::server и Styx::phy_server — альтернативные backend-профили сервера.\n\n' if ru else 'For an application using the native client, select Styx::client instead of Styx::codec: hosted lib9 already contains those codec symbols. Do not combine the two implementations in one executable or mix their native headers. Styx::server and Styx::phy_server are alternative server backend profiles.\n\n')
            text += '## ' + ('3. Подключить копию исходников' if ru else '3. Embed a source copy') + f'''\n\n```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
set({option} OFF CACHE BOOL "Minimal communication layer")
set(BUILD_TESTING OFF CACHE BOOL "Build the application without library examples")
add_subdirectory({directory}-source)
add_executable(device main.c)
target_link_libraries(device PRIVATE {package}::{component})
```

'''
            text += ('Это вариант полного CMakeLists приложения: рядом лежит скопированный каталог пакета. Cache-переменные принадлежат внешней сборке; в существующем проекте согласуйте BUILD_TESTING с его владельцем. Настройки установленных пакетов не меняют уже собранный профиль: для добавления native-компонента надо пересобрать/переустановить библиотеку.\n\n' if ru else 'This is a complete application CMakeLists with a copied package directory alongside it. Cache options belong to the outer build; coordinate BUILD_TESTING with an existing project. Discovery options cannot change an already installed build profile: adding native components requires rebuilding/reinstalling the library.\n\n')
            text += '## ' + ('Диагностика' if ru else 'Troubleshooting') + '\n\n'
            text += ('| Симптом | Что проверить |\n| --- | --- |\n| Пакет не найден | CMAKE_PREFIX_PATH и выполненную установку |\n| Версия не подходит | Запрошенную версию, EXACT и установленную minor-линию |\n| Компонент недоступен | Build option библиотеки; имя/регистр компонента |\n| C++ undefined reference | Соответствие заголовка и библиотеки; использование экспортированного target |\n| После переноса не виден header | Переносить весь prefix; не добавлять старый include-путь вручную |\n' if ru else '| Symptom | Check |\n| --- | --- |\n| Package not found | CMAKE_PREFIX_PATH and completed installation |\n| Version incompatible | Requested version, EXACT and installed minor line |\n| Component unavailable | Library build option and exact component name/case |\n| C++ undefined reference | Matching headers/library and the exported target |\n| Header missing after moving SDK | Move the whole prefix; remove hardcoded old include paths |\n')
            page(base,'docs/embedding'+suffix,text)


if __name__ == '__main__':
    main(Path(__file__).resolve().parents[1])
