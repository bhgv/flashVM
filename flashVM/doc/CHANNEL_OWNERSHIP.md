# Shared channel ownership / Совместное владение каналами

## English

[`fvm_channel_create`](../include/flashvm/channel.h#L56) returns one owned reference. [`fvm_channel_retain`](../include/flashvm/channel.h#L92) adds
an owned reference before passing the same channel to another worker or VM
context. [`fvm_channel_release`](../include/flashvm/channel.h#L96) drops one reference; [`fvm_channel_destroy`](../include/flashvm/channel.h#L100)
is retained as a compatibility alias for release. Only the last release
frees the queue, strings, and platform synchronization object.

The channel is an independent C handle, **not yet an [`FVM_Value`](../include/flashvm/value.h#L26)**. This
step enables explicit sharing across independent contexts but does not yet
implement automatic propagation through cloned tables or the `<...>` syntax.
Never release the final reference while another thread may be inside a
channel API operation. Closing wakes blocked senders and receivers; coordinate
worker shutdown and join before the final release. Received string pointers
are borrowed until the next receive on that channel.

## Русский

[`fvm_channel_create`](../include/flashvm/channel.h#L56) возвращает одну принадлежащую вызывающему ссылку.
[`fvm_channel_retain`](../include/flashvm/channel.h#L92) добавляет ссылку перед передачей того же канала другому
рабочему потоку или контексту VM. [`fvm_channel_release`](../include/flashvm/channel.h#L96) освобождает одну
ссылку; [`fvm_channel_destroy`](../include/flashvm/channel.h#L100) оставлен как совместимый с прежним API синоним.
Только последняя ссылка уничтожает очередь, строки и примитивы синхронизации.

Канал пока является отдельным C-дескриптором, **а не [`FVM_Value`](../include/flashvm/value.h#L26)**.
Автоматическое копирование таблиц с каналами и синтаксис `<...>` ещё не
реализованы. Нельзя освобождать последнюю ссылку, пока другой поток выполняет
операцию с каналом. Перед последним освобождением следует остановить потоки
и дождаться их завершения. Указатель на принятую строку действителен только
до следующего получения сообщения из этого канала.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_create` | function | [include/flashvm/channel.h:56](../include/flashvm/channel.h#L56) |
| `fvm_channel_destroy` | function | [include/flashvm/channel.h:100](../include/flashvm/channel.h#L100) |
| `fvm_channel_release` | function | [include/flashvm/channel.h:96](../include/flashvm/channel.h#L96) |
| `fvm_channel_retain` | function | [include/flashvm/channel.h:92](../include/flashvm/channel.h#L92) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
