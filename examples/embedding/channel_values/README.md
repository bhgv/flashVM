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

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_channel_value_copy`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_create`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_receive`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_release`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_channel_value_send`](../../../include/flashvm/channel_value.h) | [CHANNEL_VALUES.md#api](../../../doc/CHANNEL_VALUES.md#api) — Владеющие channel handles и явное копирование. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-channel_values).
<!-- example-catalog-api:end -->
