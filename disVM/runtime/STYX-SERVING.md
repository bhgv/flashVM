# Live per-connection Styx serving / Живой корень подключения

Copyright (c) 2026 bhgv

The intended mount-with-preparation path selects a **live ordinary directory or
namespace view for the attaching client**. It does not snapshot a subtree, copy
its contents or serialize a tree into Styx messages. The owner's preparation step
may hide, add, substitute or rebind entries according to authenticated client
identity. The selected directory then remains a normal changing service root.

При подключении с подготовкой обработчик создаёт либо выбирает отдельную папку/
представление для клиента и назначает её доступным корнем. Состав каталога и
ресурсы могут меняться во время работы; перепубликация дерева и recursive scan
не требуются. Видимость namespace и права операций дополняют друг друга.
Одна и та же информация может быть представлена разным клиентам по-разному.

## Shared root on a bidirectional mount / Общий корень встречного подключения

A bidirectional `disvm --styx-mount-fd FD --at PATH -- …` always publishes
the running program's live ordinary VFS in the reverse direction. Every
new lookup or open selects a resource with the current namespace bindings.
Existing opens keep their handles. The scoped process portal retains its
own routing. Directory enumeration in this reverse view hides reserved
StyxRPC names. Explicit protocol service paths keep their own admission.

Двунаправленный `disvm --styx-mount-fd FD --at PATH -- …` всегда публикует
живую обычную VFS запущенной программы во встречном направлении. Новый
поиск или открытие выбирает ресурс с учётом текущих привязок. Открытые
дескрипторы продолжают работать со своим ресурсом. Перечисление каталогов
во встречном дереве скрывает служебные имена StyxRPC. Явные пути протокола
открываются по правилам соответствующей службы.

[FlashVM startup / Запуск FlashVM](https://github.com/bhgv/flashVM/blob/master/flashVM/doc/STYX_RPC_PLAYER.ru.md)

## Protocol contract / Контракт протокола

| Operation | Meaning / Значение |
| --- | --- |
| Tversion/Rversion | Negotiate protocol/session and message-size bound |
| Tauth and authentication exchange | Establish verified identity where required; wire uname alone is insufficient |
| Tattach/Rattach | Associate a fid with the selected served root, returning its qid |
| Twalk/Rwalk | Resolve successive components from an unopened fid; return qids, not directory contents |
| Topen/Ropen | Check requested access and open that fid for I/O |
| Tread/Rread on directory | Read complete stat-format entries of this directory only, possibly in multiple requests |
| Tread/Twrite on open file | Operate by fid, offset and count; no mandatory implicit Twalk per chunk |
| Tstat/Rstat | Obtain attributes of one fid; directory entries already carry these attributes |
| Tclunk/Tremove | Release a fid / remove a node with the documented fid semantics |
| Tflush/Rflush | Cancel an outstanding tagged request using the documented completion rules |

A directory entry contains name, qid, mode/flags, atime, mtime, length, uid, gid,
muid and the protocol type/dev fields. A child directory appears as one entry;
listing it requires another walk/open/read. Directory read offsets follow the
protocol's sequential-entry rules. Attribute records must remain complete.
A walk of dot-dot at the exported root remains at that root. Fid numbers are local
to a connection, not global node addresses; qid identifies a server-side node.

Пространство fid общее внутри соединения. Корень и implied user относятся к
attachment; типичный обслуживаемый случай — одно подключение с его подготовленным
корнем. Несколько attachments/пользователей на одном транспорте нельзя смешивать
через один global user или client-supplied root path. После успешного attach
используются назначенные сервером root и проверенная identity, а не новые
непроверенные утверждения клиента о правах.

Twalk itself does not list a directory. It cannot walk an already-open fid.
Read/write use the previously opened fid; a fresh name lookup can discover changed
membership without reopening the entire service. An already-open fid must not
silently retarget a different node just because its old pathname is now reused.
There is no protocol requirement to walk before every read or write.

Standard open(5) checks permissions at open and preserves granted access for the
open fid. Our separately documented dynamic provider-policy revocation is an
explicit service extension, not an implication of repeated Twalk. The chosen
failure-only diagnostic Tstat policy and change notifications must not be turned
into unconditional preflight stat/walk requests on every successful data chunk.
Backend operations still decide actual success, counts and errors.

## Existing code / Существующий код

The pinned Inferno sources already contain the relevant split:

- [Styxserver.attach](../upstream/appl/lib/styxservers.b#L131) stats the configured
  rootpath and associates it with a new fid; it does not enumerate the hierarchy.
- [Styxserver.walk](../upstream/appl/lib/styxservers.b#L170) calls walk1 for each
  supplied name and rejects already-open fids.
- [Styxserver.canread/read](../upstream/appl/lib/styxservers.b#L292) uses the existing
  open fid; directory read calls Navigator.readdir for that directory, then packdir.
- [Styxserver.canwrite](../upstream/appl/lib/styxservers.b#L354) checks an open writable
  fid; it does not perform another path walk.
- [Navigator](../upstream/module/styxservers.m#L25) separates stat, walk and readdir.
- [Styx packdir/unpackdir](../upstream/appl/lib/styx.b) provides the stat wire codec.

The helpers do not implement authentication merely by copying uname from Tattach.
A transport/attachment adapter must authenticate and prepare/select the authorized
root before admitting service requests. No upstream files are changed here.

## Runtime0.136 acceptance / Проверки

[The executable guest](fixtures/styx-live/main.b) uses the actual existing matched
Styxservers handlers and Styx codec, with two independent connection instances
and live navigator services. It checks:

- Separate roots, permissions and fid tables, including identical fid numbers.
- No directory enumeration during attach or walk.
- A directory read returns only direct child entries, with decoded stat attributes.
- A child added after open appears on the next read of the same directory fid.
- A previously missing child becomes walkable from an existing unopened root fid,
  without another attach; dot-dot stays at the prepared root.
- Read-only and writable connections differ; normal file read needs no extra walk.
- Walk on an opened fid fails; Rread packing/unpacking preserves complete records.

The [driver](test_styx_live.py) executes interpreter/JIT with both GC observation
settings in dynamic and static builds. This validates the existing handlers/codec,
not a completed socket listener, network authentication or native Sys.mount adapter.
These transport/attachment integration steps remain pending. No tree snapshot API
was added. Existing Vfsexport capture/immutable explicit overlay objects retain
their documented local capability semantics; they must not be described as a
completed live namespace-to-Styx adapter.

## References / Источники

User-supplied manual indexes:
[cat-v intro](http://man.cat-v.org/inferno/5/intro) and
[Styx message descriptions](http://doc.cat-v.org/inferno/manual_pages/5/).
Those pages were unavailable to the lookup tool in this session. Semantics were
checked against the pinned source and the official Inferno manual:
[INTRO(5)](https://inferno-os.org/inferno/man/5/0intro.html),
[attach](https://inferno-os.org/inferno/man/5/attach.html),
[walk](https://inferno-os.org/inferno/man/5/walk.html),
[open](https://inferno-os.org/inferno/man/5/open.html),
[read/write](https://inferno-os.org/inferno/man/5/read.html),
[stat/wstat](https://inferno-os.org/inferno/man/5/stat.html).
The target here is Inferno's current Styx/9P2000 profile; older Styx dialects must
not be mixed into the same wire decoder without explicit version selection.


Additional user-supplied references, read alongside the official manual and source:
[Russian INTRO translation](https://blog.lhs.su/man/0intro-vvedenie-v-fajlovyj-protokol-9p-v-inferno-os/),
[Styxservers framework](https://powerman.name/Inferno/man/2/styxservers.html),
[Styx message/Dir codec](https://powerman.name/Inferno/man/2/styx.html).
The existing framework explicitly queries Navigator instead of storing the file
hierarchy; it is the integration base, not a replacement protocol to reinvent.


For the native service/mount path use the [existing Inferno C server, client and codecs](STYX-NATIVE.md).
The Limbo framework test above complements that work; it does not replace the C implementation.
