# vfs.localChannel(shape, slots)

[Русский](ASINC_LOCAL_CHANNEL.ru.md) · [VFS in tasks](ASINC_VFS.md) ·
[Implementation](../src/styxrpc/channel_local.c)

Call `vfs.localChannel(shape, slots)` or `os.localChannel(shape, slots)`
inside a detached task in the stock paired player. It creates a local
channel for a compiled Limbo parameter. The remote method's schema
determines the element type. For example, use `"i"` for `chan of int`
and `"s"` for `chan of string`. The slot count is an integer from 0 to 8,
where zero gives a rendezvous. An invalid shape or slot count fails with
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). An allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).

The returned object belongs to this task. `trySend(value)` attempts a
local enqueue and returns whether it succeeded. `tryReceive()` consumes
an element or returns `undefined` for an empty queue. `close()` retires
the whole queue and can be called repeatedly. While a DisVM call has the
queue on loan, its operations go through the reverse scope of that call.
The call retires the loan before local use resumes. It may wait for the
peer. The movie continues to serve the reverse Styx requests.

The [AS worker fixture](../tests/styxrpc/peer-asinc-scope-worker.as)
seeds one slot, passes the queue to a Dis method and reads the reply from
the Dis-owned channel. Use [`vfs.chan(path)`](ASINC_VFS.md) to open a
named exchange with dynamic values.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
