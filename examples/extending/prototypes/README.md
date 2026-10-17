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
| [`fvm_object_get`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set_prototype`](../../../include/flashvm/value.h) | [PROTOTYPES.md](../../../doc/PROTOTYPES.md) — Связывание цепочки прототипов. |

[Описание в общем каталоге](../../README.md#example-extending-prototypes).
<!-- example-catalog-api:end -->
