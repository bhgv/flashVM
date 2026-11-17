# Terminal endpoint announcements / Объявления endpoint терминала (runtime0.173)

A native host can now connect the automatically prepared
[terminal movie grant](TERMINAL-MOVIES.md) to the generic FlashVM loader through
an existing authorized Styx connection. Publication needs no Limbo monitoring
loop, guest callback or guest suspension. This is an optional native host API;
ordinary launches and existing service-file clients continue to work.

Хост теперь может связать автоматически созданный movie grant задания с общим
загрузчиком FlashVM через уже разрешённое Styx-соединение. Runtime сам обновляет
состояние объявления при запуске, смене namespace и завершении процесса. Native
I/O owner получает push-уведомление и отправляет актуальное состояние. Limbo для
этого не нужен; гостевые потоки не останавливаются.

## Authority and ordinary 9P / Права и обычный 9P

The host creates one `DVM_MovieOffer` for one job/connection and supplies it in
`DVM_TerminalChannels.offers` before submitting the launch. The descriptor retains
this native source through request, handoff and process ownership. A source can
have only one initial owner; after retirement create a new source for a new job.
No address, username, provider pointer or credential is serialized.

[`styxmovieofferroot(source, control)`](../native/movie/offer.h) returns an owned
ordinary VFS directory exposing these exact granted objects:

| Relative path | Meaning / Значение |
| --- | --- |
| `ctl` | The exact fixed control file provided by the host; native transport/queue ownership remains with the host. / Выданный управляющий stream. |
| `movie` | Read/search-only live directory; new opens list the current generation. / Живой каталог текущего поколения. |
| `movie/1`, `movie/2`, … | Current exact movie service; older names refuse new lookup/open. / Текущая служба, старые имена больше не открываются. |

Publish this directory using existing `styxtree` and authenticated/prepared
publication machinery. The receiver uses **ordinary Twalk from the already
attached root**, followed by Topen/Twrite/Tread/Tclunk. Original libstyx's
connection-scoped `aname` and username rules remain unchanged. No extra Tattach,
listener, global registry, host pathname lookup or implicit network dial exists.
The announcement name grants no access outside the supplied directory.

Каталог можно подготовить и выдать обычным путём публикации VFS. Он удерживает
исходные grants, а не копию дерева. Изменения доступных ресурсов разрешаются
обычной VFS и фактической операцией. Уже открытые объекты удерживаются независимо;
смена поколения не превращает прежний FD в другой файл. Правила доступа исходного
control provider сохраняются. Каталоги имеют `0555`, не допускают создание файлов.

## Native host lifecycle / Жизненный цикл native-хоста

| API | Contract / Контракт |
| --- | --- |
| [`dvm_movie_offer_create/retain/release`](vfs/launch/movie_offer.h) | Owned native source and explicit pins; no managed pointers. / Native source и явное владение. |
| `dvm_movie_offer_observe` | Owned changes-journal pin; subscribe using existing native notice API. Callbacks only signal native I/O readiness. / Journal pin для push-уведомлений. |
| `dvm_movie_offer_info` | Coherent latest snapshot; intermediate generations may coalesce. / Актуальное состояние под mutex. |
| [`styxmovieofferframe`](../native/movie/offer.h) | Encode tag0 declaration plus one latest state; NOT_FOUND before publication, no transport I/O. The host allocates an unused directional tag and serializes control writes. / Кодировать последнее состояние без I/O. |
| `dvm_movie_offer_bind/close` | Trusted native compare-and-swap replacement/retirement; stale generations refuse. Runtime normally owns these calls. / Замена/закрытие по точному поколению. |

Subscribe before launch, then take an initial snapshot to avoid a missed readiness
edge. Retain source and journal until admitted callbacks and I/O users retire.
Callbacks run outside the source lock. They must not enter a guest heap, SDL,
provider I/O or a blocking control write; wake the connection's native I/O owner.
That owner snapshots and encodes the latest state, then queues complete records
with the connection's other control traffic. After partial transport writes,
finish the acknowledged record prefix; never interleave record bytes.

Runtime reserves the service during adoption without advertising READY. It commits
publication after successful resident/mailbox preparation, before first guest
dispatch. A post-adoption startup failure retires the reservation as CLOSED;
a pre-adoption failure preserves handoff ownership. FORKNS/NEWNS publish the new
service only after successful namespace preparation. A spawned child gets its own
local service but no ownership of the parent's announcement source. Final process
retirement closes only the exact generation it owns.

Подписка и исходный snapshot делаются native-хостом. Каждое уведомление требует
перечитать актуальное состояние: журнал может объединить несколько изменений.
Запись в транспорт делает его собственный I/O owner. Runtime не вызывает Limbo,
не ждёт обработки уведомления и не останавливает другие рабочие потоки.

## Wire extension / Расширение содержимого stream

This is an S-record application type, **not a 9P protocol extension**.
[`announcement.h`](../native/movie/announcement.h) declares
`flashvm.movie.offer`, version1. Tag0 binds a host-selected unused tag on the
existing control direction. Every offer record contains exactly:

| Field | Encoding / Кодирование |
| --- | --- |
| State | One byte: READY=1, CLOSED=2 |
| Generation | Existing canonical positive S-integer, range1 through `0x7fffffffffffffff`; no trailing bytes |

The canonical relative path is `movie/<decimal-generation>`. Generations never
wrap. Native READY reserves one final generation for CLOSED; the upper identity
half is reserved for the projection's synthetic directory/control identities.
Older READY records cannot replace newer accepted state. Same-generation READY
is idempotent; conflicting state, invalid version/CRC/integer, unbound data,
control EOF or a terminal connection notice closes the receiver. CLOSED is sticky.
It never authorizes reconnecting or looking for another provider.

## Generic terminal receiver / Общий приёмник терминала

```c
StyxMovieSession *session = styxmoviesession(granted_client);
/* Native control I/O owner, serialized feed: */
accepted = styxmoviecontrol(session, received_bytes, received_size);
/* Owner-thread player setup, keep session alive through the run: */
loader.fetch = styxmovieannouncedfetch;
loader.userdata = session;
```

The client must already be attached to the granted directory above. Feed is
serialized by the caller and may run concurrently with fetch. Partial/coalesced
records are supported. The receiver retains directional bindings and ignores
other bound types; its individual complete control-record limit is
`MOVIE_FRAME` (8320 bytes). Route large media/bulk records through their own
channels or a host dispatcher; this API is not a general unlimited media demux.
The session observes native client disconnect automatically. Call
`styxmoviecontrolend` on control EOF; join all feed/fetch callers before
`styxmoviesessionfree`. Freeing the receiver does not shut down the host's client.

Before READY, after CLOSED or on malformed control input, fetch refuses. Each
successful fetch walks the current generation, opens one independent fid and uses
the existing movie request protocol. A generation change or disconnect observed
before completion refuses even an otherwise valid complete SWF; the managed
player discards staged bytes. There is no native-file/HTTP fallback. Normal
provider-first CODE and ordinary union-order DATA rules remain unchanged.

Сессия удерживается до возврата player. `styxmovieavailable` позволяет прочитать
состояние без I/O:1 ready,0 pending,-1 closed. Функция fetch выполняет native I/O
без входа в Dis/AS heaps. Сам player по-прежнему работает синхронно на своём
owner thread; асинхронный player API здесь не добавлен.

## Verified scope / Проверенный объём

- Runtime constructor acceptance: paired/duplex/absent terminal, interpreter/JIT;
  READY only after mailbox preparation, failed startup CLOSED without READY,
  exactly one READY/CLOSED pair and no child takeover.
- Native unit: unpublished reservation, rollback, two real pthread CAS contenders,
  canonical per-source names, live directory listing and held-open identities.
- Real original-libstyx TCP plus actual FlashVM player:34 interpreter/JIT scenarios,
  including generation update/replay, change during acquisition, control EOF,
  disconnect, bad CRC/version and refusal with a valid local SWF present.
- Normal and ASan/UBSan SDK builds; runtime dynamic/static and ASan/UBSan builds.
  LeakSanitizer is disabled in sanitizer acceptance.

The host still supplies authenticated admission, transport, native I/O scheduling
and control-write serialization. Automatic creation of an external FlashVM
process, a complete distributed launcher, general managed network/file writes,
input filtering and automatic media-channel balancing are subsequent work.
This API supplies the reusable admission/announcement/receiver path for them.
Native consumers must rebuild for the appended optional descriptor field; no
Limbo interface layout changes are introduced by runtime0.173.
