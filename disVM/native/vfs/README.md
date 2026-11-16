# VFS file-grant bridge / Адаптер файлового grant VFS

Copyright (c) 2026 bhgv

This first adapter connects a **prepared provider's root file** to independent
native opens usable by libstyx callbacks. It uses the existing VFS publication,
view, provider and precise-chunk implementations. It does not reopen host paths,
copy a namespace tree or execute/pause guest code. The original file-only constructor still refuses directory roots; the
[directory/node extension](TREE.md) accepts prepared directory capabilities.

Первый слой обслуживает **корневой файл уже выданного provider-capability**.
Каталоги теперь обслуживает [отдельный node/cursor-слой](TREE.md). Универсальный
сервер публикаций и mount-клиент сюда пока не входят.
VFS сохраняет решение по правам за самой операцией; mode — только metadata.

## Build / Сборка

```sh
cmake -S disVM/native -B build/inferno-native \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/inferno-native --parallel 4
ctest --test-dir build/inferno-native --output-on-failure
```

The opt-in `styx_vfs` target compiles owned VFS sources and the actual non-graphical
FlashVM file/filesystem/synchronization ports. `infernostyx` uses the complete
native lib9 archive; no bootstrap lib9 is linked beside it. Linux64 is required;
local execution is amd64. Other architecture execution is not claimed here.

`file_provider.h` belongs to VFS translation units; its constructor retains a
borrowed `DVM_Provider`. Inferno callbacks include only `file.h`, with opaque
capabilities and scalar metadata/results. No native struct, pointer or VFS handle
is serialized. Real 9P messages still use the original lib9 codecs and libstyx.

## Ownership / Владение

- `styxbacking(provider)` retains a prepared file capability, returning an owned
  wrapper or NULL. It never resolves a network name as a host path.
- `styxfileopen(backing, mode)` returns an independent owned open. Modes 0/1/2
  select canonical provider `r`/`wo`/`rw`; no create/truncate is requested. The selected open mode is enforced by the bridge even if a provider exposes
  additional method pointers. The
  native-filesystem provider already maps its `rw` to existing-file `rwo`.
- `styxsetaux(server, opened)` transfers that open only on return 0, while the
  current fid slot is empty and `freefid` is installed. On failure the caller
  still owns it and must close it. Set only after native open succeeds.
- `styxgetaux(server)` borrows context during dispatch. Walk clones start empty;
  they do not share or copy an opened handle. Clunk/reset/disconnect/shutdown
  consumes contexts through `freefid` before releasing client backend state.
- `styxbackingfree` drops the wrapper; existing opens independently pin provider
  and code/state lifetime. `styxfileclose` consumes an open after its serialized
  borrowers retire. The caller must not double-close or race close with I/O.

Один fid владеет одним открытием. Одинаковые Qid не означают общий cursor или
общее владение handle. Заимствования живут только в пределах native-операции;
managed pointers и callbacks Limbo здесь отсутствуют. Источник должен сам
реализовать контракт metadata открытого handle; адаптер его не подменяет.

## Operations / Операции

Offsets remain 64-bit at the boundary and are checked against the underlying
native-long seek API before conversion. The first target is Linux LP64. A
nonseekable stream accepts only its next sequential offset; a mismatch refuses
without pretending to rewind. Failed seek poisons further I/O because a generic
provider might have unknown cursor effects. Calls on an open are serialized by
its owner; this module does not stop other workers or acquire namespace locks.

`StyxTransfer` preserves confirmed prefix, success, uncertainty and native stop
cause. A known positive prefix can be encoded as a short Rread/Rwrite even when
that chunk ends with refusal. Zero-byte failed writes are errors, not EOF.
Unknown effects are never reported as successful progress and poison further I/O
on that open. No chunk is replayed or automatically moved to another provider.
The existing VFS chunk layer performs diagnostic metadata probes only after
failed data operations; metadata does not convert failure into permission.

Никакого предварительного stat на каждый chunk не добавлено. Открытие проверяет
тип полученного объекта, а сами read/write определяют успех и реальное число
байтов. Политика provider может меняться динамически. Info запрашивается через
удержанный handle; она не гарантирует больше, чем metadata-контракт backend.

## Acceptance and scope / Проверки и границы

The native TCP fixture obtains a real local publication ticket/grant, closes the
publisher and original provider reference, and serves only that retained file
capability using original libstyx. Three scenarios cover:

1. Two independently positioned opens of one Qid; a write acknowledges exactly
   three bytes before revocation, later writes return Rerror, live mode/data
   changes are observed, unpublishing preserves old opens, and one clunk does not
   close another fid. Exactly two writes reach the provider.
2. Uncertain write effects return Rerror and poison that handle. A repeated write
   never reaches the provider; another open can still read confirmed live data.
   Exactly one write reaches the provider.
3. A read-only publication ceiling refuses ORDWR before provider open, permits
   OREAD and refuses Twrite without calling the data writer. No write reaches
   the provider. Held reads survive unpublishing.

Each scenario checks exact open/close counts and final factory destruction. The
control endpoint and static alias belong only to the test fixture, not a public
service API. Packet usernames do not authenticate a peer. The publication request
is local, explicitly authorized before TCP starts; remote provenance bits are
never taken from network input. Per-connection native admission and pending owner review are now covered in
[PEER.md](PEER.md); live directory serving is covered in [TREE.md](TREE.md).
Runtime0.144 adds [deferred Rattach and attach flush](DEFERRED.md). Transport
authentication, full metadata mapping and a general mount client are outside
this bridge change. These interfaces do not
freeze that design or a prepared tree.

Графики, FreeType и альтернативного wire-кодека в адаптере нет. Новые
`styxgetaux`/`styxsetaux`/`Styxops.freefid` — расширение native C API Inferno;
сообщения Styx/9P2000 неизменны. Пересобрать потребителей с новым Client/Styxops.


Opaque byte endpoints may omit handle info. Their factory metadata is used only
for the initial file-type check, not fabricated as fstat; `styxfileinfo` remains
unsupported. Directory opens require handle metadata for identity validation.
Обычные view-handles теперь получают metadata удержанного объекта с текущим
policy/mode/name masking; составные виртуальные родители сохраняют view-defined
семантику. [Каталоговые API и ограничения](TREE.md).

## Connection admission / Admission соединения (runtime0.143)

[PEER.md](PEER.md) documents submit-only publication endpoints and per-connection
root selection, ownership, native preparation and current synchronous libstyx
attach limits. `vfs-peer-wire` verifies isolation with equal wire users and Qids.

## Deferred attach / Отложенный attach (runtime0.144)

[Native deferred Rattach](DEFERRED.md) now preserves pending tag/fid until a
reviewer signals the socket owner. Flush/reset/disconnect/shutdown retire the
request before any later reply; no client retry or guest suspension is required.
The original Inferno implementation and VFS TCP tests cover dedicated native
review, pipe wakeup, cancellation, denied publication close and shutdown.
