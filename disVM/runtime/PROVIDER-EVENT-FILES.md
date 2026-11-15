# Optional provider journal view and ORDWR — runtime 0.130

Copyright (c) 2026 bhgv

The automatic per-thread notification path is [system inboxes](SYSTEM-EVENTS.md).
It needs neither this optional projection nor a Limbo observer. This view is an
explicit host-installed diagnostic capability with independent refresh tickets.

Автоматические сообщения потокам описаны отдельно. Ни этот optional view, ни
Providerwatch не нужны для подписки потока на изменения его namespace.

## Existing read/write opens / Открытие existing read/write

`Sys.open(path, Sys->ORDWR)` now uses the normal namespace/native-provider open
path. Native files use the portable [fvm_port_file_open](../../flashVM/ports/file_port.h)
mode `rwo`: existing files only, no truncation, creation or append; read/write share
one logical offset beginning at zero. Seek and restore preserve that shared offset.
Legacy `rw` file-channel append semantics remain distinct. Provider callbacks get
`rw`, requiring both effective access directions during MAUTORWX selection.

ORDWR поддерживается через обычную VFS, не через отдельный путь. Existing native
file открывается без create/truncate/append, с единым read/write cursor с нуля.
Нет файла — отказ. Ограничения платформенных directory anchors остаются прежними.

## Trusted host projection / Trusted host projection

[dvm_provider_eventfs](vfs/eventfs.h) takes a borrowed source provider and returns
an owned factory or NULL. It retains the source journal until all view factories
and opened handles retire. The trusted host may mount it through the existing
namespace installation API. The root lists a single `events` file. Bind aliases
require the ordinary stable-bind promise; unmount affects future lookup, not
already opened descriptor capabilities.

`dvm_provider_eventfs` не даёт новых прав: source должен явно разрешить READ
корня через access callback. При отсутствии callback — отказ. Проверка повторяется
при каждой операции; metadata mode не заменяет effective access decision.
Ни source data open, ни source stat не нужны для чтения journal view.

Each ORDWR open owns its own observer, selected class mask and captured ticket.
Reading returns exactly one complete experimental record or temporary unavailability.
A short buffer fails without splitting the record. Readn also stops at one record.
Rereading an unacknowledged diagnostic ticket may repeat it: unlike the automatic
system inbox, this is a refresh/ack view, not a per-change FIFO history.

Каждый FD имеет независимый cursor. Explicit ack подтверждает только versions,
захваченные прочитанным ticket. Изменение во время reread остаётся dirty. Смена
session делает старый ack недействительным. Filter change не подтверждает unseen
classes. Partial, unknown и batched control writes отклоняются до изменения state.

[dvm_changes_ack](vfs/changes.h) validates a same-journal captured native ticket,
committing only selected class watermarks under the journal mutex. Its inputs
are trusted native bookkeeping, not guest-supplied pointers or authorization.
Refresh callbacks use that same acknowledgement implementation.

The optional pure [Providerwatch module](../module/providerwatch.m) initializes Sys,
opens an explicitly installed view, reads one diagnostic ticket, changes class
selection and acknowledges a delivered ticket after caller state reread. Unsigned
IDs stay opaque fixed-width hex strings; there is no numeric narrowing or polling
wait/sleep helper. A failed initial record handshake refuses an ordinary file
before sending any control write to it.

Providerwatch — optional reader, не механизм автоматического наблюдения. Он не
регистрирует поток и не приостанавливает его; `next` делает одно неблокирующее
чтение, returning event/idle/error. Временная недоступность не превращается в EOF.

**All textual records/control commands and this new helper interface are
experimental, pending format agreement.** No permanent Styx/9P extension is assigned.
**Формат и новый helper API предварительные.** Native source session/generation
bookkeeping не означает реализованный сетевой heartbeat или remote fid recovery.

## Executable checks / Исполняемые проверки

[Native projection tests](vfs/test_eventfs.c) cover independent cursors, undersized
buffers, replay, ticket races, masks, access revocation, reconnect and source lifetime.
[Anchor-port checks](vfs/test_anchor_port.c) exercise native rwo overwrite/shared
cursor, seek/restore and absent-file refusal. The [guest driver](test_provider_watch.py)
runs the real Sys interface, pure Limbo helper and system device in C/JIT with both
GC observation settings, and independently checks native bytes remain `ABXDE` while
the ordinary wrong-protocol file, bound alias target and absent path stay intact.
