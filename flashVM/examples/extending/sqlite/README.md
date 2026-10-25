# Extending: SQLite (optional adapter implemented)

This C89 example creates an in-memory SQLite database and queries it. It is built when SQLITE is enabled in DEFS, using the bundled SQLite dependency. The `.as` script describes a **future host-registered function**, not a built-in AVM1 API. Never expose unrestricted SQL or arbitrary file paths to untrusted SWF; prefer host-authorized databases and parameterized statements. This tutorial exercises `exec` and an integer query; see the linked adapter guide for its scope.

<!-- example-catalog-api:start -->
## Назначение и особенности

Открывает SQLite в памяти, выполняет SQL и получает целочисленный результат. Показывает владение adapter handle; доступность определяется поддержкой SQLite, а AS-файл описывает отдельный host bridge.

**Текущая CMake-цель:** `flashvm_examples_extending_sqlite_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_sqlite_close`](../../../include/flashvm/sqlite.h#L30) | [SQLITE_API.md](../../../doc/SQLITE_API.md) — SQLite adapter и запросы. |
| [`fvm_sqlite_exec`](../../../include/flashvm/sqlite.h#L22) | [SQLITE_API.md](../../../doc/SQLITE_API.md) — SQLite adapter и запросы. |
| [`fvm_sqlite_open`](../../../include/flashvm/sqlite.h#L18) | [SQLITE_API.md](../../../doc/SQLITE_API.md) — SQLite adapter и запросы. |
| [`fvm_sqlite_query_int`](../../../include/flashvm/sqlite.h#L26) | [SQLITE_API.md](../../../doc/SQLITE_API.md) — SQLite adapter и запросы. |

[Описание в общем каталоге](../../README.md#example-extending-sqlite).
<!-- example-catalog-api:end -->

**Профиль сборки:** необходим [`SQLITE`](../../../cmake/BuildDefinitions.cmake#L10) в DEFS. При отключённой возможности
CMake вообще не создаёт цель этого примера. Профиль `cmake/profiles/agg.defs`
включает её; `minimal.defs` отключает.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sqlite_close` | function | [include/flashvm/sqlite.h:30](../../../include/flashvm/sqlite.h#L30) |
| `fvm_sqlite_exec` | function | [include/flashvm/sqlite.h:22](../../../include/flashvm/sqlite.h#L22) |
| `fvm_sqlite_open` | function | [include/flashvm/sqlite.h:18](../../../include/flashvm/sqlite.h#L18) |
| `fvm_sqlite_query_int` | function | [include/flashvm/sqlite.h:26](../../../include/flashvm/sqlite.h#L26) |
| `SQLITE` | DEFS switch | [cmake/BuildDefinitions.cmake:10](../../../cmake/BuildDefinitions.cmake#L10) |
<!-- source-links:end -->
