# Shared channel ownership / Совместное владение каналами

## English

`fvm_channel_create` returns one owned reference. `fvm_channel_retain` adds
an owned reference before passing the same channel to another worker or VM
context. `fvm_channel_release` drops one reference; `fvm_channel_destroy`
is retained as a compatibility alias for release. Only the last release
frees the queue, strings, and platform synchronization object.

The channel is an independent C handle, **not yet an `FVM_Value`**. This
step enables explicit sharing across independent contexts but does not yet
implement automatic propagation through cloned tables or the `<...>` syntax.
Never release the final reference while another thread may be inside a
channel API operation. Closing wakes blocked senders and receivers; coordinate
worker shutdown and join before the final release. Received string pointers
are borrowed until the next receive on that channel.

## Русский

`fvm_channel_create` возвращает одну принадлежащую вызывающему ссылку.
`fvm_channel_retain` добавляет ссылку перед передачей того же канала другому
рабочему потоку или контексту VM. `fvm_channel_release` освобождает одну
ссылку; `fvm_channel_destroy` оставлен как совместимый с прежним API синоним.
Только последняя ссылка уничтожает очередь, строки и примитивы синхронизации.

Канал пока является отдельным C-дескриптором, **а не `FVM_Value`**.
Автоматическое копирование таблиц с каналами и синтаксис `<...>` ещё не
реализованы. Нельзя освобождать последнюю ссылку, пока другой поток выполняет
операцию с каналом. Перед последним освобождением следует остановить потоки
и дождаться их завершения. Указатель на принятую строку действителен только
до следующего получения сообщения из этого канала.
