# Inferno C implementation reuse / Использование C-кода Inferno

Copyright (c) 2026 bhgv

The native integration must start from Inferno's existing C implementation, not a
new wire protocol or tree serializer. The source paths below were inspected at
our pinned revision7f2241455734e1142a3254fad9d1ec4f94032ac7. The new runtime0.136
acceptance links the **original C codecs already present in our bootstrap**.
The C server and client have been read but are not yet integrated as a native
service/mount adapter; do not confuse the codec test with that integration.

Нужная C-основа уже есть. Предыдущая проверка Limbo Styxservers не заменяет перенос
C-сервера и клиента. Берём существующие codec/dispatch/RPC реализации; наш слой
должен соединять их с VFS, transport ports и native lifetime, а не повторять 9P.

| Source | Role and integration / Назначение и интеграция |
| --- | --- |
| [tools/libstyx/styxserver.c](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/tools/libstyx/styxserver.c) | C server: clients, fid state, dispatch, callbacks |
| [include/styxserver.h](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/include/styxserver.h) | Styxops callbacks, Client.u per-client context, server lifecycle |
| [tools/libstyx/Posix.c](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/tools/libstyx/Posix.c) | Original socket/select platform boundary; map to our host ports |
| [emu/port/devmnt.c](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/emu/port/devmnt.c) | Client mount RPC, walk/open/read/write, tags/multiplexing/flush; kernel-bound Chan/Mnt interface needs adaptation |
| [emu/port/styx.c](https://github.com/bhgv/Inferno-OS-bhgv/blob/7f2241455734e1142a3254fad9d1ec4f94032ac7/emu/port/styx.c) | Kernel-side protocol conversion implementation |
| [C lib9 codecs](../../tools/limbo-bootstrap/CMakeLists.txt#L79) | convM2S/convS2M, convM2D/convD2M already build from the same pinned upstream |

Styxops supplies attach, walk, open, create, read, write, close, remove, stat and
wstat callbacks. The prepared live root belongs to client context; callback handlers
resolve only within that authority. No namespace snapshot or upfront enumeration
is required. For local calls, use the same ordinary VFS provider operations.

The C client confirms the protocol ordering: mntwalk emits Twalk; mntread and
mntwrite call mntrdwr with Tread/Twrite using an existing fid. Directory data caching
is explicitly disabled in mntread. Listing is read, not an implicit tree walk.

## Adaptation checks / Границы адаптации

The source is reusable, but its host/kernel assumptions must be handled explicitly.
For example, the inspected server declares Fid.fid and findfid/newfid/fidclone
arguments as short although Fcall.fid/newfid are32-bit. It also passes the address
of the32-bit count field as ulong*. Full-width fid and LP64 callback-count regressions
are required before enabling it as our native server. These are source-review
findings, not fixes made in this checkpoint. Confirmed changes must be tested in
the owning upstream repository and then imported through vendor-sync.

The client uses Chan/Mnt, locks, queues, error-stack and channel lifetime machinery
from Inferno. Adapt that boundary to our dedicated native execution/transport
contracts; do not import the cooperative scheduler or stop guest workers globally.
C transport/backend callbacks must never execute managed Limbo on a foreign thread.
Authentication and client-root preparation belong before authorized service access;
copying uname from a packet does not authenticate it.

## Original C codec acceptance / Проверка исходных C-кодеков

[styx/test_codec.c](styx/test_codec.c) links limbo_bootstrap_lib9, without copying or
rewriting its codecs. Independent fixed Twalk bytes check full32-bit fids/newfids,
UTF-8 and every truncated prefix. Tread checks the fixed layout with64-bit offset.
A stat record inside Rread checks high-bit64-bit qids, permissions, length, times,
name, uid/gid/muid and refusal of a partial record. Native dynamic/static tests
complement the [existing Limbo server-handler checks](STYX-SERVING.md).
No C-server listener, native mount client or complete wire/authentication service
is claimed by these codec and handler tests.
