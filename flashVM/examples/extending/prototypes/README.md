# Extending: prototype-backed native objects

The embedding example in `../../embedding/prototypes/` shows the underlying
object API. Extensions can create objects and set their prototype through the
same C ABI. Registration of native constructors and instance methods is still
planned; this folder is a documented design placeholder, not a working plugin.

<!-- example-catalog-api:start -->
## Назначение и особенности

Заметка о построении native-объектов на прототипах и ссылка на embedding/prototypes. Это описание повторного использования C ABI, без отдельной реализации загружаемого плагина.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_object_get`](../../../include/flashvm/value.h#L102) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set_prototype`](../../../include/flashvm/value.h#L90) | [PROTOTYPES.md](../../../doc/PROTOTYPES.md) — Связывание цепочки прототипов. |

[Описание в общем каталоге](../../README.md#example-extending-prototypes).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_object_get` | function | [include/flashvm/value.h:102](../../../include/flashvm/value.h#L102) |
| `fvm_object_set_prototype` | function | [include/flashvm/value.h:90](../../../include/flashvm/value.h#L90) |
<!-- source-links:end -->
