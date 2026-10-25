# Каналы с явным владением: API и ограничения

## API

[`FVM_ChannelValue`](../include/flashvm/channel_value.h#L17) — небольшой C89-дескриптор, содержащий `FVM_Channel *`.
[`fvm_channel_value_create`](../include/flashvm/channel_value.h#L24) создаёт канал с одной ссылкой.
[`fvm_channel_value_from_channel`](../include/flashvm/channel_value.h#L28) удерживает существующий канал.
[`fvm_channel_value_copy`](../include/flashvm/channel_value.h#L32) создаёт **нового владельца того же канала**:
сообщения и очередь не копируются. [`fvm_channel_value_release`](../include/flashvm/channel_value.h#L37) освобождает
одну ссылку и обнуляет дескриптор. [`fvm_channel_value_send`](../include/flashvm/channel_value.h#L41) и
[`fvm_channel_value_receive`](../include/flashvm/channel_value.h#L46) выполняют блокирующие операции канала.

### `fvm_channel_value_create`

`out` — ненулевой адрес свободного дескриптора. Создаёт новую очередь без ограничения ёмкости с одной ссылкой. Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) для NULL или [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46) при сбое выделения памяти/создания синхронизации либо [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) при сбое выделения идентификатора канала. При неуспехе с допустимым out его channel равен NULL.

### `fvm_channel_value_from_channel`

`out` — свободный дескриптор, `channel` — действующий канал. Увеличивает счётчик ссылок и только при успехе записывает указатель в out. Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) или [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) при NULL либо переполнении счётчика. Не передаёт и не отнимает исходную ссылку; при ошибке out не изменяется.

### `fvm_channel_value_copy`

`out` — отдельный свободный дескриптор, `source` — действующий владеющий дескриптор. Удерживает дополнительную ссылку на тот же канал, а не копирует очередь. Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) или [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) для NULL/пустого source либо переполнения счётчика. При ошибке out не изменяется. Самокопирование и перезапись активного out теряют учёт удержанной ссылки; не используйте их.

### `fvm_channel_value_release`

`value` — дескриптор, чью ссылку нужно освободить. Возвращаемого значения нет. NULL и пустой дескриптор допускаются. После освобождения channel обнуляется; последний release уничтожает канал и его очередь. Нельзя освобождать последний reference во время операций других потоков. Это не операция close: при наличии других владельцев канал остаётся открыт.

### `fvm_channel_value_send`

`value` — действующий дескриптор, `message` — примитивное значение. Для очереди копирует значение (и байты строки), не забирая владение оригиналом. Блокируется при заполнении ограниченной очереди до освобождения места или закрытия. Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) для неверных указателей/типа/NULL-строки, [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) для metatable, [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46) при выделении памяти или [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) при закрытом канале. Для файловых/удалённых каналов делегирует underlying send и передаёт его ошибки сериализации/ввода-вывода.

### `fvm_channel_value_receive`

`value` — действующий дескриптор, `message` — ненулевой адрес результата. Для очереди ожидает сообщение (также ждёт снятия заморозки); закрытая очередь может отдать оставшиеся примитивы. Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) для неверных указателей, [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) при закрытой пустой/замороженной очереди, [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) для graph packet во главе очереди (он не извлекается). Результат читают только после [FVM_OK](../include/flashvm/flashvm.h#L44). Строка заимствована до следующего успешного чтения из того же канала или его уничтожения; дополнительная ссылка не продлевает жизнь этой строки через чтение. Для нескольких получателей используйте owned receive из channel API. Файловые/удалённые каналы передают статусы underlying receive; файловый EOF не ждёт внешнего писателя.

Обёртки не входят в VM, не вызывают ActionScript и не создают GC roots. Синхронизация очереди защищает внутреннее состояние канала, но не одновременное изменение одного дескриптора. Удерживайте ссылку на всё время операции; согласовывайте чтение заимствованных строк между получателями.

## Владение и потоки

Каждый успешный вызов create/from/copy требует ровно одного release.
Нельзя перезаписывать активный дескриптор: это приведёт к утечке ссылки.
Нельзя освобождать последнюю ссылку одновременно с операциями другого потока.
Перед освобождением дескриптора, переданного через `userdata`, дождитесь
`worker_join`. Закрытие канала пробуждает ожидающие потоки. Полученная строка
заимствована: она действительна до следующего чтения из того же канала
либо до уничтожения канала.

## Пока не реализовано

Дескриптор **не** является значением [`FVM_Value`](../include/flashvm/value.h#L26) и не отслеживается GC AVM1.
Его нельзя напрямую хранить в свойстве ActionScript, сериализовать в таблицах
AVM1 или передавать через планируемый синтаксис `<...>`. `userdata` при
создании рабочего потока не копируется: временем жизни управляет хост.
Примеры: `examples/embedding/channel_values`, `channel_cpp98` и
`examples/extending/channel_worker`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../src/capi/channel_c.c#L30) |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:32](../include/flashvm/channel_value.h#L32) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:24](../include/flashvm/channel_value.h#L24) |
| `fvm_channel_value_from_channel` | function | [include/flashvm/channel_value.h:28](../include/flashvm/channel_value.h#L28) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:46](../include/flashvm/channel_value.h#L46) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:37](../include/flashvm/channel_value.h#L37) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:41](../include/flashvm/channel_value.h#L41) |
| `FVM_ChannelValue` | struct | [include/flashvm/channel_value.h:17](../include/flashvm/channel_value.h#L17) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
