# Bundled dependency sources / Встроенные исходники зависимостей

All third-party sources are now under `external/`; `extern/` has been removed.
CMake compiles these sources locally. It does not download dependencies, use
pkg-config, or search for a system Cairo, Pixman, SQLite, zlib or AGG installation.

| Directory | Pinned version | When compiled |
| --- | --- | --- |
| mini_regex | Existing project implementation | Always |
| zlib | 1.3.1 | Always, for bounded CWS loading |
| agg | AGG 2.4 core at the recorded fork commit | `GRAPHICS_AGG` |
| cairo | 1.16.0, portable memory-image profile | `GRAPHICS_CAIRO` |
| pixman | 0.38.4, scalar core with thread-local storage | With Cairo |
| sqlite | 3.50.4 amalgamation | `SQLITE` (default) |

The compatibility pins are deliberate, not a claim to ship the latest upstream
releases. Cairo 1.18.4 requests C11 in its upstream build; this checkpoint instead
validates the 1.16.0 image profile with documented C89 adaptations. SQLite keeps
its full 64-bit ABI; LP64 uses C89 `long`, other targets need a native compiler or
system 64-bit type. Actual execution is validated on Linux x86_64 only.

Each new dependency has `PROVENANCE.json` with the official archive URL and
SHA-256, upstream licensing notices, and `flashvm.patch` containing every local
source change. `SOURCES.sha256` covers the vendored upstream files. See
[build keys](../doc/BUILD_DEFINITIONS.md) and
[dependency policy](../doc/EXTERNAL_DEPENDENCIES.md).

Все сторонние исходники объединены в `external/`; папка `extern/` удалена.
Библиотеки собираются из включённых исходников без сетевых загрузок и без
поиска системных установок. Растеризатор выбирается в корневом `DEFS.txt`.
По умолчанию включены AGG и SQLite; Cairo/Pixman собираются при выборе
`GRAPHICS_CAIRO`. Версии зафиксированы ради проверяемой совместимости, а не как
утверждение об актуальности upstream. Лицензии, происхождение, контрольные суммы
и патчи сохранены. Подробнее: [ключи сборки](../doc/BUILD_DEFINITIONS.ru.md).
