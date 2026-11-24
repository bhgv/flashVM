# styx: встраивание и установленный SDK

Copyright (c) 2026 bhgv

[README](../README.ru.md) · [Introduction](introduction.ru.md) · [Tutorial](tutorial.ru.md) · [API](index.ru.md)

## Что выбрать

Установленный SDK отделяет сборку библиотеки от приложения. Копия каталога исходников позволяет менять адаптеры или выбирать MCU toolchain. Оба пути используют собственные исходники пакета: DisVM, FlashVM и соседняя библиотека не нужны. Нативная проверка выполнена на Linux amd64; это не приёмка всех физических устройств и ОС.

## 1. Собрать и установить библиотеку

```sh
cmake -S . -B build-sdk -DSTYX_HOSTED=OFF -DBUILD_TESTING=OFF
cmake --build build-sdk -j
cmake --install build-sdk --prefix "$PWD/sdk"
```

Команды выполняются из корня скопированного пакета. OFF выбирает минимальный профиль. Для нативного слоя используйте ON. BUILD_TESTING=OFF исключает учебные программы и их test-only зависимости. Для hosted Styx Python всё ещё нужен при выборе исходников lib9.

## 2. Подключить SDK из отдельного приложения

```sh
mkdir ../device-example
cp examples/codec.c ../device-example/main.c
```

Сохраните следующий текст как ../device-example/CMakeLists.txt. main.c — полный рабочий пример пакета, а не отсутствующая заготовка.

```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
find_package(Styx 0.1 CONFIG REQUIRED COMPONENTS codec)
add_executable(device main.c)
target_link_libraries(device PRIVATE Styx::codec)
set_target_properties(device PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
```

```sh
cmake -S ../device-example -B ../device-example/build -DCMAKE_PREFIX_PATH="$PWD/sdk"
cmake --build ../device-example/build
../device-example/build/device
```

CMAKE_PREFIX_PATH — корень установленного SDK, не include-каталог и не исходный репозиторий. После переноса всего SDK укажите новое место. Include-пути и транзитивные библиотеки задаёт target; не записывайте старый абсолютный путь вручную. Поддерживается CMAKE_INSTALL_INCLUDEDIR, в том числе api/include. Потребитель с C++ включает язык CXX в project и может выбрать CXX_STANDARD 98. C linkage клиента Styx сохраняет C ABI при вызове из C++.

## Версия и компоненты

Версия пакета — 0.1.0. Запрос 0.1 выбирает совместимую установленную версию той же minor-линии; минимальная запрошенная версия всё равно соблюдается. EXACT требует точного совпадения, иная minor/major-линия не принимается автоматически. Это правило обнаружения SDK, а не заявление о wire-версии или ABI всех будущих выпусков.

| Компонент | Target | Доступность |
| --- | --- | --- |
| `codec` | `Styx::codec` | `всегда` |
| `wire` | `Styx::wire` | `всегда` |
| `nine` | `Styx::nine` | `STYX_HOSTED=ON` |
| `server` | `Styx::server` | `STYX_HOSTED=ON` |
| `carrier` | `Styx::carrier` | `STYX_HOSTED=ON` |
| `walk` | `Styx::walk` | `STYX_HOSTED=ON` |
| `client` | `Styx::client` | `STYX_HOSTED=ON` |
| `phy_server` | `Styx::phy_server` | `STYX_HOSTED=ON` |

```cmake
find_package(Styx 0.1 CONFIG REQUIRED
    COMPONENTS codec OPTIONAL_COMPONENTS carrier)
if(Styx_carrier_FOUND)
    target_link_libraries(device PRIVATE Styx::carrier)
endif()
```

Это отдельный вариант конфигурации вместо предыдущего find_package. REQUIRED-компонент, отсутствующий в сборке, останавливает конфигурацию с причиной. Отсутствующий OPTIONAL_COMPONENT не мешает использованию минимального слоя. Неизвестное имя также отклоняется, если оно REQUIRED.

Для приложения с native client выбирайте Styx::client вместо Styx::codec: hosted lib9 уже содержит те же codec symbols. Не соединяйте эти две реализации в одном executable и не смешивайте их native заголовки. Styx::server и Styx::phy_server — альтернативные backend-профили сервера.

## 3. Подключить копию исходников

```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
set(STYX_HOSTED OFF CACHE BOOL "Minimal communication layer")
set(BUILD_TESTING OFF CACHE BOOL "Build the application without library examples")
add_subdirectory(styx-source)
add_executable(device main.c)
target_link_libraries(device PRIVATE Styx::codec)
```

Это вариант полного CMakeLists приложения: рядом лежит скопированный каталог пакета. Cache-переменные принадлежат внешней сборке; в существующем проекте согласуйте BUILD_TESTING с его владельцем. Настройки установленных пакетов не меняют уже собранный профиль: для добавления native-компонента надо пересобрать/переустановить библиотеку.

## Диагностика

| Симптом | Что проверить |
| --- | --- |
| Пакет не найден | CMAKE_PREFIX_PATH и выполненную установку |
| Версия не подходит | Запрошенную версию, EXACT и установленную minor-линию |
| Компонент недоступен | Build option библиотеки; имя/регистр компонента |
| C++ undefined reference | Соответствие заголовка и библиотеки; использование экспортированного target |
| После переноса не виден header | Переносить весь prefix; не добавлять старый include-путь вручную |

[Ожидание срока в хостовом сервере](reference/styxphywait.ru.md).
