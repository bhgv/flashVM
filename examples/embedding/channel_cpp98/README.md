# C++98 channel wrapper / Обёртка канала на C++98

A minimal RAII wrapper releases its owned channel when leaving scope.
Copying is deliberately disabled to avoid accidental double-release;
explicitly call `fvm_channel_value_copy()` when another owner is required.

Минимальная RAII-обёртка освобождает канал при выходе из области видимости.
Копирование запрещено, чтобы избежать двойного освобождения. Для создания
нового владельца вызывайте `fvm_channel_value_copy()`.

<!-- example-catalog-api:start -->
## Назначение и особенности

Оборачивает владеющий дескриптор канала в C++98 RAII-класс. Запрещает неявное копирование и освобождает ссылку при выходе из области видимости; полезен для безопасного управления каналом в C++-хосте.

**Текущая CMake-цель:** `flashvm_examples_embedding_channel_cpp98_main`.


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
| [`fvm_value_boolean`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

Локальная RAII-обёртка вызывает C API владения; её запрет копирования не является новым ABI VM. См. [CHANNEL_VALUES.md](../../../doc/CHANNEL_VALUES.md#api).

[Описание в общем каталоге](../../README.md#example-embedding-channel_cpp98).
<!-- example-catalog-api:end -->
