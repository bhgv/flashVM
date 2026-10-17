# Каналы с явным владением: API и ограничения

## API

[`FVM_ChannelValue`](../include/flashvm/channel_value.h#L17) — небольшой C89-дескриптор, содержащий `FVM_Channel *`.
[`fvm_channel_value_create`](../include/flashvm/channel_value.h#L22) создаёт канал с одной ссылкой.
[`fvm_channel_value_from_channel`](../include/flashvm/channel_value.h#L24) удерживает существующий канал.
[`fvm_channel_value_copy`](../include/flashvm/channel_value.h#L26) создаёт **нового владельца того же канала**:
сообщения и очередь не копируются. [`fvm_channel_value_release`](../include/flashvm/channel_value.h#L29) освобождает
одну ссылку и обнуляет дескриптор. [`fvm_channel_value_send`](../include/flashvm/channel_value.h#L31) и
[`fvm_channel_value_receive`](../include/flashvm/channel_value.h#L34) выполняют блокирующие операции канала.

## Владение и потоки

Каждый успешный вызов create/from/copy требует ровно одного release.
Нельзя перезаписывать активный дескриптор: это приведёт к утечке ссылки.
Нельзя освобождать последнюю ссылку одновременно с операциями другого потока.
Перед освобождением дескриптора, переданного через `userdata`, дождитесь
`worker_join`. Закрытие канала пробуждает ожидающие потоки. Полученная строка
заимствована: она действительна до следующего чтения из того же канала
либо до уничтожения канала.

## Пока не реализовано

Дескриптор **не** является значением [`FVM_Value`](../include/flashvm/value.h#L24) и не отслеживается GC AVM1.
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
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:26](../include/flashvm/channel_value.h#L26) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:22](../include/flashvm/channel_value.h#L22) |
| `fvm_channel_value_from_channel` | function | [include/flashvm/channel_value.h:24](../include/flashvm/channel_value.h#L24) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:34](../include/flashvm/channel_value.h#L34) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:29](../include/flashvm/channel_value.h#L29) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:31](../include/flashvm/channel_value.h#L31) |
| `FVM_ChannelValue` | struct | [include/flashvm/channel_value.h:17](../include/flashvm/channel_value.h#L17) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
