# Optional SQLite API (C89)

`flashvm/sqlite.h` exposes [`fvm_sqlite_open`](../include/flashvm/sqlite.h#L16), [`fvm_sqlite_exec`](../include/flashvm/sqlite.h#L18), [`fvm_sqlite_query_int`](../include/flashvm/sqlite.h#L20) and [`fvm_sqlite_close`](../include/flashvm/sqlite.h#L22). It uses SQLite's official C API and is compiled only when the dependency is detected. It is not automatically accessible to ActionScript: register native callbacks explicitly and enforce file-path/SQL permissions in the host. See `examples/extending/sqlite`. SQLite API reference: https://sqlite.org/capi3ref.html

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_sqlite_close` | function | [include/flashvm/sqlite.h:22](../include/flashvm/sqlite.h#L22) |
| `fvm_sqlite_exec` | function | [include/flashvm/sqlite.h:18](../include/flashvm/sqlite.h#L18) |
| `fvm_sqlite_open` | function | [include/flashvm/sqlite.h:16](../include/flashvm/sqlite.h#L16) |
| `fvm_sqlite_query_int` | function | [include/flashvm/sqlite.h:20](../include/flashvm/sqlite.h#L20) |
<!-- source-links:end -->
