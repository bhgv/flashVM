# Приёмка независимых библиотек

Copyright (c) 2026 bhgv

[English](VALIDATION.md) · [Руководство](README.ru.md)

Среда проверки: Linux amd64, GCC, CMake и Python3. Выбор исходников Linux arm64/riscv64
существует, но исполнение на этих физических платформах здесь не проверялось.

| Проверка | Результат |
| --- | --- |
| Hosted Styx | 16/16: шесть исходных серверных регрессий, portable codec, пять моделей каталогов |
| Hosted libunibus | 11/11: nested records, native waits, четыре order regression, ошибки инициализации очередей, gateway, edge cases, ошибки startup gateway, partial/failed I/O |
| Межбиблиотечная связь | 5/5 моделей через независимо слинкованный шлюз |
| Изолированные копии | Пакеты настроены/собраны/проверены вне исходного каталога |
| Минимальные профили | Styx codec/wire без hosted adapters; libunibus wire без native queues |
| Перенос SDK | Prefix перемещён; внешние C/C++98 programs используют codec, wire, native client и queues с CMAKE_INSTALL_INCLUDEDIR=api/include |
| Документированные приложения | Примеры из установленного SDK собраны как внешние приложения во всех четырёх профилях; вместе с C/C++98 probes — 19 executable |
| Поиск SDK | Exact version принята; несовместимая major/minor и отсутствующие required components отклонены; отсутствующие optional components допустимы |
| Санитайзеры | ASan/UBSan hosted tests и пять межбиблиотечных сценариев пройдены; LSan отключён в этом sandbox |
| Документация offline | Оба языка, Markdown-ссылки, точные source anchors, независимые viewers и изоляция путей |
| Корневой viewer | Восемь существующих регрессий пройдены с его заявленными Python-зависимостями |
| Связь с canonical source | 224 Styx / 37 libunibus экспортированных файлов сверены с подготовленными canonical algorithms |

## Воспроизведение

Из корня репозитория:

```sh
python3 lib/export.py --check
python3 lib/tests/isolated.py /absolute/path/to/cmake
python3 lib/tests/isolated.py /absolute/path/to/cmake --sanitize
python3 lib/tests/documentation.py
python3 lib/tests/reference_scanner.py
```

Рядом с переданным cmake нужен соответствующий ctest. isolated.py копирует пакеты
под build/, собирает их и сохраняет вывод команд. Межбиблиотечную проверку можно
запустить отдельно, передав два уже существующих build-каталога:

```sh
python3 lib/tests/interoperability.py STYX_BUILD UNIBUS_BUILD
```

## Изоляция отказов шлюза

Проверяются CLI/два настоящих порта, частичный startup, pending RPC при работе другого
клиента, FIFO-ошибки неверного client frame, обычный 9P Rerror без разрыва connection,
sticky backend failure/усечение/неверная длина, повреждение outer traffic и потеря
stdout при открытом stdin, также при другом pending RPC.

Native wrappers проверяют освобождение startup-ресурсов, продолжение только остатка
после EINTR/коротких операций/EOF, различие backend timeout и stdout backpressure,
terminal output после частичной неуспешной записи. Регрессионная проверка повторного
использования вывода падает на исходном io.c именно с этой причиной; с terminal latch
она проходит.

Source anchors проверяются не только на диапазон: функция и точная строка примера
должны находиться по ссылке. Fixtures генератора исключают имена из comments/strings
при выборе declaring header и реального call site.

## Комментарии и границы доказательств

В базовых примерах и C-модулях gateway — авторские пояснения. Команда
`python3 -m lib.tools.example_lines` проверяет markers и известные шаблонные фразы,
но не сочиняет комментарии и не доказывает их семантическую точность. Source inventory
остаётся metadata; не каждый унаследованный комментарий объявляется прошедшим ручной review.

C++98-проверка воспроизвела прежний undefined reference native client до добавления
C linkage в canonical header и его проверяемую копию. Ошибки allocation, barrier и
thread creation в примере очередей проверяются отдельно от gateway startup failures.

Это не приёмка физических SPI/I2C/CAN/USB-драйверов, всех ОС, готового MCU-сервера или
всех возможностей основного брокера. Сверка токенов не доказывает отсутствие всех
ошибок. Внешние библиотеки не требуют VM; ссылки на основную справку необязательны.

## Приёмка собственного носителя

Настоящий native 9P-клиент и исходный Styx-сервер обмениваются через ограниченную память без descriptor-backed carrier. Проверены передачи по 2/3 байта, backpressure, дочитывание перед EOF, неопределённый результат записи, конечное monotonic wait, 500 конкурентных wake и четыре стадии отказа конструктора. Linux link-time guard запрещает descriptor/network entry points; это не трассировка системных вызовов ядра. Перемещённый SDK собирает и запускает полный пример из установленных исходников, а также C/C++98 consumers PHY-сервера.
