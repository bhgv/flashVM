# extending/meta_call

README ссылается на tests/meta_call.c: объекты связываются метатаблицей и __call, принимающим таблицу аргументов. Показывает соглашение meta-call; отдельного main в examples нет.

## Реализация и запуск

Это концептуальная заметка или AS-эскиз без отдельного C main.
Смотрите связанное руководство и комментарии исходника; текст AS требует внешнего компилятора.

<!-- example-catalog-api:start -->

## Связанные функции C API

Исходник сценария находится в [tests/meta_call.c](../../../tests/meta_call.c).
Он демонстрирует следующие операции; в этой папке нет собственного main.

- [`fvm_meta_call`](../../../include/flashvm/meta_call.h#L20) — явный вызов __call: [API_INDEX.md, Native calls and metatables](../../../doc/API_INDEX.md#native-calls-and-metatables).
- [`fvm_meta_register`](../../../include/flashvm/metamethod.h#L20) — регистрация native-метаметода: [METAMETHODS.md](../../../doc/METAMETHODS.md).
- [`fvm_table_set_metatable`, `fvm_table_get_index`](../../../include/flashvm/table.h) — метатаблица и чтение аргументов: [описание соглашения](README.ru.md).
- [`fvm_object_create`, `fvm_value_number`](../../../include/flashvm/value.h) — объект и числовые ячейки: [API_INDEX.md](../../../doc/API_INDEX.md#values-and-borrowed-views).

[Описание в общем каталоге](../../README.md#example-extending-meta_call).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](../../../include/flashvm/meta_call.h#L20) |
| `fvm_meta_register` | function | [include/flashvm/metamethod.h:20](../../../include/flashvm/metamethod.h#L20) |
| `fvm_object_create` | function | [include/flashvm/value.h:62](../../../include/flashvm/value.h#L62) |
| `fvm_table_get_index` | function | [include/flashvm/table.h:22](../../../include/flashvm/table.h#L22) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:24](../../../include/flashvm/table.h#L24) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
