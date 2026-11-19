# Standalone library acceptance

Copyright (c) 2026 bhgv

[English guide](README.md) · [Русское руководство](README.ru.md)

Acceptance environment: Linux amd64, GCC, CMake and Python3. The source-level
Linux arm64/riscv64 selections have not been executed on those physical targets.

| Check | Result |
| --- | --- |
| Styx hosted | 12/12: six original server regressions, portable codec, five catalog models |
| libunibus hosted | 8/8: nested records, independent native waits, four order regressions, partial-initialization failure injection, gateway |
| Cross-library wire integration | 5/5 catalog models through independently linked gateway |
| Isolated copies | Each package configured, built and tested outside its original source directory |
| Minimal profiles | Styx codec/wire without hosted adapters; libunibus wire without native queues |
| Relocated SDK | Installed prefixes moved; external C and C++98 consumers linked/ran codec, wire, native client and queues with CMAKE_INSTALL_INCLUDEDIR=api/include |
| Documented applications | Installed example sources built/ran as external applications in all four profiles; 16 downstream executables including C/C++98 probes |
| SDK discovery | Exact version accepted; incompatible major/minor and missing required components rejected; unavailable optional components accepted |
| Sanitizers | ASan/UBSan hosted tests plus five cross-library cases passed; LSan was disabled in this sandbox |
| Offline documentation | Both languages, local Markdown links, physical source anchors, live independent viewers and path confinement |
| Workspace documentation viewer | 8 existing regression tests passed with its declared Python dependencies |
| Canonical relationship | 224 Styx / 37 libunibus exported source files verified against prepared canonical algorithms |

Reproduction from the repository root:

```sh
python3 lib/export.py --check
python3 lib/tests/isolated.py /absolute/path/to/cmake
python3 lib/tests/isolated.py /absolute/path/to/cmake --sanitize
python3 lib/tests/documentation.py
python3 lib/tests/interoperability.py build/standalone-styx-final build/standalone-libunibus-final
```

The last command accepts two existing build directories. `isolated.py` copies the
packages under `build/`, performs their builds and records command output there.
It requires the matching `ctest` beside the supplied `cmake` executable.

## Границы доказательств

Проверены реальные нативные библиотеки, независимый wire-клиент, пять моделей
серверов и их обмен через S-записи. Проверка токенов сохраняет алгоритм после
оформления, но не доказывает семантическую полноту комментариев или отсутствие
всех ошибок. Это также не приёмка физического SPI/I2C/CAN/USB драйвера, всех ОС,
готового MCU-сервера или всех функций основного брокера. Внешние библиотеки не
должны требовать исходники/сборку VM; ссылки на основную документацию необязательны.

## Reviewed tutorial maintenance

The nested-record and native-queue examples now carry authored operation-specific contracts. `python3 -m lib.tools.example_lines` checks their review markers and known stock prose; it does not manufacture comments or certify their semantics. Formatting preserves explicitly reviewed contracts. The remaining source inventory is metadata, not a statement that every inherited comment has received manual review.

C++98 acceptance reproduced the former native-client undefined reference before adding C linkage to the canonical client header and its checked export. Initialization failure injection checks both engine allocation failures, barrier initialization and thread creation; constructed resources must retire in all four cases.
