# Flash UCS2 casing data

`tables.rs` is an unmodified copy of Ruffle `wstr/src/tables.rs` at commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`. It records Flash Player's one-unit
uppercase/lowercase mappings, including historical differences from Unicode.
We use the MIT option of the upstream dual license, reproduced in `LICENSE.md`.

Run `python scripts/generate_flash_case.py` to regenerate `case_table.hpp` from
the bundled source, without network access. Only the C++98 header is compiled.
The table is used for loaded AVM1 contexts; unloaded host casing retains the
existing Unicode 3.2 data in `external/unicode`.

Данные регистра Flash UCS2 взяты из указанного фиксированного коммита Ruffle
без изменения исходной Rust таблицы. Выбрана MIT лицензия upstream. Заголовок
C++98 генерируется из включённого файла без сети; Rust compiler не нужен.
Загруженный AVM1 использует Flash mappings, незагруженный host — Unicode 3.2.
