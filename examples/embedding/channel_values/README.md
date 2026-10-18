# Owned channel handles / Каналы с управляемым владением

This C89 example creates a channel, copies its *handle* (not its queue), sends
one number and receives it through the second handle. Each handle must be
released exactly once. Build using `FVM_BUILD_EXAMPLES=ON`.

Пример на C89 создаёт канал, копирует *дескриптор* (не очередь), передаёт число
и принимает его через второй дескриптор. Каждый дескриптор освобождается один раз.

**ActionScript design sketch (not yet implemented):** `var c = <>; c <- 17;`

**Эскиз ActionScript (пока не реализован):** `var c = <>; c <- 17;`

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт владеющий channel handle, явно копирует ссылку на тот же канал и передаёт число между двумя handles. Объясняет, почему копия дескриптора не является копией очереди и требует отдельного release.

**Текущая CMake-цель:** `flashvm_examples_embedding_channel_values_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке описано действие каждой
функции и дан переход к руководству; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_channel_value_copy`](../../../include/flashvm/channel_value.h#L26) | Удерживает дополнительную ссылку на тот же канал; очередь и сообщения не копируются. Каждую успешную копию нужно освободить. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_copy](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_copy). |
| [`fvm_channel_value_create`](../../../include/flashvm/channel_value.h#L22) | Создаёт новый внутрипроцессный канал с одной владеющей ссылкой; сохраняет дескриптор в out. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_create](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_create). |
| [`fvm_channel_value_receive`](../../../include/flashvm/channel_value.h#L34) | Получает примитивное сообщение; ждёт при пустой очереди. Полученная строка заимствована у канала. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_receive](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_receive). |
| [`fvm_channel_value_release`](../../../include/flashvm/channel_value.h#L29) | Освобождает одну ссылку и обнуляет дескриптор; последний release уничтожает канал, а не просто закрывает его. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_release](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_release). |
| [`fvm_channel_value_send`](../../../include/flashvm/channel_value.h#L31) | Копирует примитивное сообщение в общий канал; ждёт при заполненной ограниченной очереди. Исходное значение остаётся у вызывающего. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_send](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_send). |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-channel_values).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:26](../../../include/flashvm/channel_value.h#L26) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:22](../../../include/flashvm/channel_value.h#L22) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:34](../../../include/flashvm/channel_value.h#L34) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:29](../../../include/flashvm/channel_value.h#L29) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:31](../../../include/flashvm/channel_value.h#L31) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
