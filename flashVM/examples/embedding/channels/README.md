# In-process channels / Внутренние каналы

**English.** `main.c` is a compiled C89 embedding example using `flashvm/channel.h`. It sends a number and receives it through a FIFO. Channels are independent of a VM context, making them useful as a transport primitive between isolated interpreters once worker-context execution is implemented. The channel owns copied string messages. The receive operation blocks until a message arrives or the channel is closed. `destroy` requires that no other threads are accessing the channel.

**Русский.** `main.c` — компилируемый пример на C89. Число отправляется через FIFO-канал и считывается обратно. Канал не привязан к контексту VM; в будущем это позволит связывать независимые интерпретаторы. Строки копируются. Чтение блокируется до получения сообщения либо закрытия канала. Перед уничтожением нужно дождаться завершения всех потоков, использующих канал.

`channel.as` contains **proposed syntax only**; the AVM1 interpreter does not yet parse channel expressions or operators.

`channel.as` показывает **проектируемый синтаксис**, который пока не исполняется AVM1.

<!-- example-catalog-api:start -->
## Назначение и особенности

Создаёт внутрипроцессную FIFO, отправляет и получает значение, затем уничтожает канал. Это базовый транспортный пример; сопровождающий channel.as описывает отдельный синтаксический эскиз.

**Текущая CMake-цель:** `flashvm_examples_embedding_channels_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_channel_close`](../../../include/flashvm/channel.h#L40) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_create`](../../../include/flashvm/channel.h#L23) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_destroy`](../../../include/flashvm/channel.h#L49) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_receive`](../../../include/flashvm/channel.h#L55) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_channel_send`](../../../include/flashvm/channel.h#L53) | [CHANNELS.md#implemented-api](../../../doc/CHANNELS.md#implemented-api) — FIFO, передача значений и закрытие. |
| [`fvm_value_number`](../../../include/flashvm/value.h#L42) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-channels).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_close` | function | [include/flashvm/channel.h:40](../../../include/flashvm/channel.h#L40) |
| `fvm_channel_create` | function | [include/flashvm/channel.h:23](../../../include/flashvm/channel.h#L23) |
| `fvm_channel_destroy` | function | [include/flashvm/channel.h:49](../../../include/flashvm/channel.h#L49) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:55](../../../include/flashvm/channel.h#L55) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:53](../../../include/flashvm/channel.h#L53) |
| `fvm_value_number` | function | [include/flashvm/value.h:42](../../../include/flashvm/value.h#L42) |
<!-- source-links:end -->
