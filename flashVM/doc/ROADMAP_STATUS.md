# FlashVM stage status / Состояние стадий — v0.762

This is a bounded review of existing implementation reports and the current
event and global-function work. It does not replace a complete per-API/SWF conformance matrix
or certify unexecuted platforms. All nine stages remain open. Existing test
counts are historical unless explicitly rerun for this revision.

Это сводка существующих отчётов и текущих событий и глобальных функций, а не
полный повторный аудит всех API. Все девять стадий открыты. Старые результаты
тестов не выдаются за новый прогон; проценты и сроки из них не выводятся.

v0.762: Explicit Zig Linux/musl toolchain builds the complete minimal core and selected programs for ARM64, RISC-V64 and x86-64; target-built static SDL/AGG player links for ARM64. x86-64/musl runtime checks pass on this x86-64 Linux host. [EN](CROSS_LINUX_ZIG.md) / [RU](CROSS_LINUX_ZIG.ru.md). ARM64/RISC-V64 execution, physical devices and OS/platform acceptance remain open.

v0.761: Desktop SDL playback prepares up to 64 PCM header segments, changes source rate/channel/width without clearing accepted audio, skips empty headers under a fixed worker budget and re-primes later cursors after navigation. [EN](PLAYER_PCM_STREAMS.md) / [RU](PLAYER_PCM_STREAMS.ru.md). Codec/mixing/drift/device/platform acceptance remains open.

v0.760: Native output continues across exhausted PCM converters without clearing queued samples or clocks; explicit cross-source frame bind preserves staged seek failure semantics. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). Host owns source ordering; desktop multi-segment integration follows separately.

v0.759: Intrinsic explicit-navigation tokens detect same-frame select/entry between host observations; SDL stream clears stale queue state under the native mutex. [EN](PLAYER_PCM_STREAMS.md) / [RU](PLAYER_PCM_STREAMS.ru.md). Cyclic per-instance token, not an event log/content identity or completed Flash seeking semantics.

v0.758: Actual desktop CLI plays one root PCM stream through an owner-authorized native SDL worker. [EN](PLAYER_PCM_STREAMS.md) / [RU](PLAYER_PCM_STREAMS.ru.md). Exact FWS/CWS accepted PCM checks use dummy audio; mixed/codecs/multi-header/drift/physical acceptance remain open.

v0.757: Intrinsic MovieClip playback observations expose selected frame, entry and play flags without script getters or allocation. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). An observation is not a navigation history or device acceptance.

v0.756: Actual MovieClip frames authorize native PCM through stop/play, frame gaps and explicit backward seek. Controlled AVM1 frame actions run in interpreter/native JIT; C --timeline tutorial emits exact converted bytes. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). No automatic desktop audio scheduling/device/reference acceptance.

v0.755: Native output frame gates hold future source blocks and pending retries without decoding. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). Native timeline authorization primitive, not automatic player audio synchronization.

v0.754: Actual C export runs converter→bounded native FIFO→explicit file consumer after source VM teardown. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). Queue retries stay separate from irreversible file writes; no desktop stream scheduling/device acceptance.

v0.753: Caller-buffered native stream output with exact pending retries, backpressure and dequeue clocks. [EN](STREAM_PCM_OUTPUT.md) / [RU](STREAM_PCM_OUTPUT.ru.md). Device/timeline acceptance remains open.

v0.752: Direct C tutorial converts prepared stream PCM to 48000 Hz stereo after VM teardown; four native workers independently convert shared immutable bytes. [EN](STREAM_PCM_CONVERSION.md) / [RU](STREAM_PCM_CONVERSION.ru.md). Scheduling/device/codecs/reference acceptance remain open.

v0.751: Native explicit rate/channel conversion preserves rational phase across prepared PCM blocks. [EN](STREAM_PCM_CONVERSION.md) / [RU](STREAM_PCM_CONVERSION.ru.md). No device/timeline playback acceptance.

v0.750: C cursor tutorial exports actual PCM after source VM teardown; four native workers independently read one immutable segment. [EN](STREAM_PCM_READER.md) / [RU](STREAM_PCM_READER.ru.md). Scheduling, codecs, device/platform/reference acceptance remain open.

v0.749: Allocation-free frame-aware PCM reader over prepared native stream snapshots. [EN](STREAM_PCM_READER.md) / [RU](STREAM_PCM_READER.ru.md). Root stream playback/scheduling remain open.

v0.748: Actual verified media reception feeds host-timed feedback; UDP/TCP loopback measurements drive another real FILE/LIVE player export. [EN](RECEIVE_WINDOWS.md) / [RU](RECEIVE_WINDOWS.ru.md). No production listener or foreign runtime acceptance.

v0.747: Bounded unique receive windows, exclusive repair categories and frozen tag-zero feedback. [EN](RECEIVE_WINDOWS.md) / [RU](RECEIVE_WINDOWS.ru.md).

v0.746: Replay explicit channel feedback through actual frame export, ACK-bound profile selection and independent FILE/LIVE UDP/TCP checks. [EN](CHANNEL_ADAPTATION.md) / [RU](CHANNEL_ADAPTATION.ru.md). No production network sampler or completed platform acceptance.

v0.745: Choose bounded bulk profiles from authenticated channel feedback. See [slot contracts](CHANNEL_ADAPTATION.md) / [контракты](CHANNEL_ADAPTATION.ru.md).

v0.744: Exercise negotiated frame parts with real player clients and transport loss. See [slot contracts](PLAYER_FRAME_PARTS.md) / [контракты](PLAYER_FRAME_PARTS.ru.md).

v0.743: Export negotiated media parts from immutable player frame leases. See [slot contracts](PLAYER_FRAME_PARTS.md) / [контракты](PLAYER_FRAME_PARTS.ru.md).

v0.742: Negotiate per-tag compact and bulk limits through tag-zero services. See [slot contracts](MEDIA_SLOTS.md) / [контракты](MEDIA_SLOTS.ru.md).

v0.741: Bounded file and live media fragment reception. See [slot contracts](MEDIA_SLOTS.md) / [контракты](MEDIA_SLOTS.ru.md).

v0.740: Fixed S-record recovery slots. See [slot contracts](MEDIA_SLOTS.md) / [контракты](MEDIA_SLOTS.ru.md).

v0.739 runs the live player readers with independent negotiated msize values
256 and 384. Full reply-file reads reassemble S records across Rread boundaries.

v0.738 adds bounded packed rectangle reads from immutable session-owned frame leases.
Independent cropped-pixel checks cover padded rows and invalid/oversized regions.

v0.737 verifies two independent 9P clients acquiring, reading and releasing real
player RGBA frames. Each old image is checked while a newer frame is published;
final pins and metadata remain readable after runner teardown. Existing pixel,
interpreter/native-JIT, file/memory/HTTP navigation and cancellation fixtures are
retained. This is an in-memory transport integration, not a production listener,
OS mount, video codec or completed roadmap-stage acceptance.

v0.737: два 9P клиента читают пиксели настоящих кадров порциями; старые leases
и финальный кадр переживают смену изображения и runner teardown. Все стадии
роудмапа остаются открытыми.

v0.736 adds a bounded copied RGBA resource with three pixel buffers, explicit
session-owned leases and chunked reads. Old leased frames remain immutable;
backpressure drops export without blocking playback. Allocation/identity/lifetime
contracts and independent Python metadata/queue vectors pass; the capture tutorial
adds an explicit optional frame budget. No codec, OS mount or network listener
is introduced, and complete player/platform acceptance remains open.

v0.736: кадры ресурсов с удержанием, поколениями и чтением порциями; native
ownership и память ограничены. Все девять стадий остаются открытыми.

v0.735 verifies actual offscreen SWF playback through 9P operation files, with
two independent state observers, source navigation epochs and cached READ after
runner teardown. Interpreter and native-JIT fixtures compare real raster pixels,
paused redraws, FILE/MEMORY/HTTP loading and joined cancellation of active child
loads. The test transport is in-memory; no production network listener or mount
is implied. All nine stages still require their full acceptance matrices.

v0.735: команды настоящего плеера идут через 9P файлы, два наблюдателя читают
независимые состояния, включая смену корня и cached READ после runner teardown.
Проверка растров interpreter/JIT не заменяет полную SWF/OS приёмку.

v0.734 adds a native player-state resource: copied root epochs, source prefixes,
completed tick/draw counters and independent READY/TICK/changed-policy events.
Quiet turns do not fill queues. The C89 lifecycle/codec contract and frame-capture
resource example pass; no VM pointer enters the payload and no new transport or
OS mount is installed. Full player/SWF/platform acceptance remains open.

v0.734: копии состояния плеера через ресурсы, поколения корней и события
завершённых тиков. Тихие TURN подавлены; владение, усечение и очистка явные.
Все девять стадий остаются открытыми.

v0.733 adds a bounded 9P2000 device-message adapter with fid walks, file I/O,
stat directories, deferred reads, flush and clunk/reset cleanup. Independent
wire tests include actual fragmented TCP loopback; a real AVM1 stop/frame/resume
runs through two 9P connections. Host transport/authentication and OS mounts
remain separate; full 9P/Inferno and roadmap-stage acceptance are not claimed.

v0.733: device-адаптер 9P2000, ожидающие read/flush и очистка fids. Проверены
независимые wire vectors, TCP loopback и AVM1 отладчик через два соединения.
Production listener, OS mount и полная приёмка стадий остаются открытыми.

v0.732 adds portable synthetic resource files: incarnation directories, authorized
operation files and independent binary event streams. Native C89 fixtures cover
partial I/O, revocation, cancellation and bounded exhaustion. No OS mount or
network transport is implied; stages 6/7/9 and all other stages remain open.

v0.732: переносимые файлы ресурсов с поколениями, операциями и событиями.
Проверены частичные чтения, отзыв прав, отмена и лимиты; OS mount и сетевой
транспорт пока не добавлены. Приёмка стадий остаётся открытой.

v0.731 implements an embeddable hybrid UDP/TCP idle/receive policy and compact
S-record hints: slower independent TCP checks, authenticated UDP probe requests,
coalesced/rate-limited reconnect advice, replay/epoch checks and wrapping clocks.
Это исполняемая политика/кодек, не UDP/TCP listener, STUN, multicast или измерение
NAT/энергозатрат. Все девять стадий остаются открытыми.
See [EN](HYBRID_HEARTBEAT.md) / [RU](HYBRID_HEARTBEAT.ru.md).

v0.729--v0.731 verification / Проверка серии:
36 selected Linux C89/C++98 checks pass, including resource/event/wire/heartbeat,
real debugger stops and two client threads, interpreter/JIT/player rasters and
four existing debugger regressions. The final three heartbeat checks were rerun
after adding delayed-advice/CRC-prefix cases. Seven shared-player checks pass
with HTTP providers disabled. New event/debugger/heartbeat C paths pass ASan/
UBSan with leak enumeration disabled; underlying VM/SDL remain ordinary builds.
ARM64 Linux, RISC-V64 Linux and Windows x64 cross-link the C89 protocol/heartbeat
contract and compile the debugger C module/public C++98 boundary. This is neither
foreign player execution nor full OS/device, NAT/multicast, Styx/9P or SWF acceptance.
Нативная проверка ограничена Linux; cross-build не означает запуск на другой ОС.
Все девять стадий остаются открытыми.


v0.730 connects actual VM breakpoint callbacks to the resource command/event layer:
two observers, cached frames, one controller claim and exact-stop resume. All
integer fields retain the compact canonical S-integer profile. Interpreter, JIT
fallback and subsequent native entries are checked where the host supports JIT.
Five focused native checks pass. Four C debugger/protocol fixtures also pass
ASan/UBSan with leak enumeration disabled; the underlying VM/SDL build is ordinary.
The two-client POSIX fixture uses a blocking owner condition-variable service.
Отладчик получает команды через двоичный интерфейс; сетевой listener, Styx/9P,
[FVM_Channel](../src/capi/channel_c.c#L26) graph bridge и полный экспорт locals/objects пока не реализованы.
See [EN](RESOURCE_DEBUGGER.md) / [RU](RESOURCE_DEBUGGER.ru.md). All stages remain open.

v0.729 adds independent bounded resource observers, filtered metadata discovery,
revocation-safe watches and atomic quiesced-session cleanup. Three real POSIX
consumers account for every publication as received or explicitly missed.
Это прикладная рассылка, не сетевой multicast, mount/9P или полная GUI-модель.
Ни один этап не закрыт. See [EN](RESOURCE_EVENTS.md) / [RU](RESOURCE_EVENTS.ru.md).

v0.728 delivers resource pause/resume/step/stop at existing player owner boundaries.
Bound binary requests control real interpreter/JIT SWFs, with exact rasters, pause
refresh, file/memory/HTTP root navigation and resource STOP cancellation/join of
an active HTTP child load. The annotated capture example adds --resource-control
for native C delivery without mandatory serialization. Replies acknowledge command
delivery, not later frame execution; STEP retains existing coalesced permits.
No remote socket service, mounted filesystem, general event stream or conversion
of every VM interface class is claimed. [EN](RESOURCE_DISPATCH.md) /
[RU](RESOURCE_DISPATCH.ru.md).

Final v0.726–v0.728 verification: 21 selected native Linux protocol/queue/player/
tutorial checks pass. Shared host variants each pass seven focused checks, with
socket HTTP and with no HTTP provider (explicit unavailable-provider local/memory
coverage). Four POSIX producers deliver 200 copied jobs with owner-only callbacks
and ordinary mutex backpressure. ASan/UBSan cover protocol/queue/control-adapter
C modules, allocation/reentry/grant/generation/cancellation contracts, threaded
producers, real interpreter/JIT resource SWFs and tutorial output/errors. Leak
checks are disabled because traced /proc enumeration is unavailable; VM, SDL and
transports remain uninstrumented. C89 protocol/owner-control contract sources
cross-link for Linux ARM64, Linux RISC-V64 and Windows x86-64; C++98 boundary clients
compile. These foreign binaries, full foreign player SDKs and devices are not
executed here. SDK uses separately built static SDL 2.30.0. Windowless tests use
an invalid video driver; ordinary window regressions use explicit dummy video.
An initially broad test-name filter selected unbuilt unrelated resource fixtures;
the final bounded selection above passes. Source links and example comment/API/
catalog checks pass. All nine roadmap stages remain open.

v0.727 adds direction-bound binary request/reply envelopes without a wire session
claim. CRC/schema validation precedes queue effects; independent vectors check
trusted identity, prefixes and rejection. Four real POSIX producer threads deliver
200 copied jobs to one owner with host-mutex backpressure. No transport/mount or
distributed atomicity is implied. All stages remain open.

v0.726 adds a bounded native owner dispatcher with copied FIFO control requests,
server-defined rights, session grants, generation-safe resource release, queued
cancellation and retained replies. Allocation failure, reentry, capacity, revoke
and stale-generation contracts are checked without VM/device calls. No native
filesystem mount or automatic VM-class conversion is claimed. All stages remain
open. [EN](RESOURCE_DISPATCH.md) / [RU](RESOURCE_DISPATCH.ru.md).

v0.725 adds an annotated standalone C89 duplex/file tutorial, C++98 ABI client,
independent transcript checks and bounded malformed-input stress. The accepted
three-layer resource/filesystem architecture is recorded explicitly as future
work: no dispatcher, VM-class conversion, mount/9P server or Inferno conformance
is claimed. [EN](RESOURCE_NAMESPACE.md) / [RU](RESOURCE_NAMESPACE.ru.md).

Final v0.722–v0.725 verification: seven native protocol/tutorial checks pass,
including independent Python big integers/zlib, length/checksum boundaries,
full 256-slot bindings, atomic rejection and arbitrary fragments. Three existing
channel/graph-transfer/serialization tests pass separately. A fresh minimal VM
static SDK builds; the C++98 protocol client links and runs against both the
standalone C library and VM archive. ASan/UBSan cover all three protocol C modules,
50000 bounded malformed/mutation cases, CRC-valid random metadata bodies and
memory/file tutorial failures. Leak checks are disabled in the traced environment;
VM and system libraries are not instrumented in this focused run. The standalone
C89 codec/tutorial cross-link for Linux ARM64, Linux RISC-V64 and Windows x86-64;
C++98 public ABI clients compile for those targets, without foreign execution.
Cross tools use a local self-path adapter because /proc/self/exe is unavailable;
this does not establish native target acceptance. Source links, API/tutorial
comments and catalog sorting pass. The provisional adaptive checksum retains
six bits for one-byte lengths; authentication remains another layer. Existing
VM wire serialization is unchanged. All roadmap stages remain open.

v0.724 adds caller-buffered incremental S-record reception. Fragment/coalescing
oracles verify exact consumption, header quotas, sticky data errors and stable
verified views through explicit reset. No transport reads/allocations/VM calls
are hidden. All stages remain open. [EN](SRECORD.md) / [RU](SRECORD.ru.md).

v0.723 adds immutable direction-local tag-zero schema declarations with bounded
caller-owned registries. Opposite tag-3 identities, idempotence, copied metadata,
atomic multi-entry conflict/capacity rejection and malformed/CRC checks pass.
No globally fixed numeric type table or automatic object execution is introduced.
All stages remain open. [EN](SRECORD.md) / [RU](SRECORD.ru.md).

v0.722 adds a standalone C89 binary S-record codec: canonical big-endian
signed-magnitude integers with last-octet bit, bounded record framing and a
provisional adaptive CRC profile. Independent Python integer/zlib vectors pass,
including prefixes, 64-bit limbs, damaged tags, nested/binary payloads and unchanged
error outputs. No VM channel/scene/9P integration is implied. All stages remain
open. [EN](SRECORD.md) / [RU](SRECORD.ru.md).

v0.721 extends the annotated capture tutorial with --offscreen and all-pixel PPM
checks using an invalid video driver. Existing windowed commands remain the
default. [EN](../examples/player/frame_capture/README.md) /
[RU](../examples/player/frame_capture/README.ru.md).

Final v0.719–v0.721 verification: 16 selected Linux static-host tests pass,
including windowless/ordinary capture, C++98 linkage, control policy and keyboard/
pointer/clip input adapters. The opt-in window/presentation tests were run with
explicit dummy video; no physical display acceptance is claimed. Shared hosts
pass nine focused checks with native socket HTTP and nine with no provider;
the latter report unavailable HTTP while exercising local/embedded paths.
ASan/UBSan pass for C player/window/frame plus timer-only/live-player/tutorial
fixtures. Leak checks are disabled because the traced runtime cannot enumerate
/proc threads; VM, SDL and transports remain uninstrumented. Public C89/C++98
clients compile for ARM64, RISC-V64 and Windows x86-64: compile-only ABI evidence,
not full foreign SDK linkage/execution. SDL 2.30.0 is a separately built static
SDK, with real native video/device validation still open. Source-link and example
comment/catalog checks pass. No roadmap stage is closed.

v0.720 exposes two additive windowless C runners using the established source,
control, observer and loader contracts. Real rasters, pause/step, root navigation
and joined transfer failure cleanup are checked with video deliberately unavailable;
host SDL events/references survive the complete run. Native-window APIs retain
behavior. SDL timing/audio dependencies remain; no GUI widget/input injection,
foreign OS runtime or stage completion is claimed. [EN](OFFSCREEN_PLAYER.md) /
[RU](OFFSCREEN_PLAYER.ru.md).

v0.719 introduces a timer-only presentation adapter with checked raster extents,
no video initialization, no cursor mutation and no SDL event-queue consumption.
The focused native test checks a deliberately unavailable video driver and host
subsystem/event ownership. This is infrastructure for windowless player output;
all roadmap stages and foreign native device acceptance remain open.

v0.718 adds a standalone annotated C89/AS/SWF capture tutorial: copy a selected
completed host tick, stop normally, and encode PPM only after SDK teardown.
Independent all-pixel file/memory/HTTP output tests include early-source and
output/argument errors. Native/shared/cross build instructions accompany it.
All roadmap stages remain open.

v0.717 integrates C frame observers with FILE/MEMORY/URL runners, pause redraws,
whole-root counters and background loading. Actual interpreter/JIT pixels and
callback ordering pass, including source navigation, null control and joined
active-transfer cleanup on observer failure. C++98 host ABI checks pass. All
roadmap stages remain open.

v0.716 adds C89 frame-observer contracts and allocation-free checked RGBA row
copying. Independent raw-byte tests verify padding, complete source/destination
extents, overflow and untouched invalid outputs without VM/SDL startup. All
roadmap stages remain open.

Final v0.716–v0.718 verification: 65 selected native frame/loading/control/mailbox
checks and 13 additional audio/timer/redraw/final-frame/build-info regressions
pass. Shared host configurations pass 35 socket-TLS, 33 curl, 27 no-provider
and 21 codec-disabled checks. ASan/UBSan cover the C frame/player/transfer/
audio/control modules and copy/live-player/tutorial fixtures, including descriptor
aliasing, overflow and independent all-pixel output. Leak detection is disabled
because traced /proc enumeration is unavailable; VM, SDL and transports are not
instrumented. Cached native mailbox/shared allocation executables encountered execute-permission
failures; fresh relinking and rerunning the affected selections passes. A renamed detached component freshly
builds and runs the file/embedded PPM tutorial with no HTTP provider. Audio/image-
enabled player and tutorials cross-link for Linux ARM64, Linux RISC-V64 and
Windows x86-64; foreign binaries are not executed here. Generated SWF/include
assets reproduce byte-for-byte; source links and tutorial comment/catalog checks
pass. Captures observe the logical software raster and still require SDL native
presentation. Native foreign OS/device, windowless embedding and full reference
acceptance remain open, as do all nine roadmap stages.

v0.715 adds a standalone C89/AS/SWF tutorial with a separate native controller,
confirmed step acknowledgements and explicit done/join/release/shutdown on every
runner exit. FILE/MEMORY/HTTP and early-error tutorial regressions accompany
native/cross build instructions. All stages remain open.

Final v0.713–v0.715 verification: 61 selected native loading/control/mailbox checks
and 13 additional audio/timer/redraw/final-frame/build-info regressions pass.
Shared host configurations pass 31 socket-TLS, 29 curl, 23 no-provider and 17
codec-disabled checks. ASan/UBSan instrument the C mailbox/player/transfer/
audio/control modules and their native allocation/concurrency/live-player/tutorial
fixtures; leak detection is disabled because the traced environment cannot perform
LeakSanitizer /proc thread enumeration. VM, SDL and transports remain uninstrumented.
A renamed detached VM component freshly builds/runs the no-provider tutorial.
Audio-enabled player and the new C tutorial cross-link for Linux ARM64, Linux
RISC-V64 and Windows x86-64 without foreign execution. Source links, individual API
example tables/comments, catalog and deterministic SWF/linked-byte checks pass.
Native foreign OS/device, full reference/debugger, race-detector/endurance
acceptance and all nine roadmap stages remain open.

Русский: прошли 74 выбранные native-проверки и четыре shared-конфигурации,
C-only ASan/UBSan без LeakSanitizer, свежая сборка отделённого компонента и три
кросс-линковки. Нативный запуск других ОС/устройств и завершение пункта 9 не заявлены.

v0.714 binds native mailbox intent to owner playback/load-monitor callbacks.
Real controller-thread interpreter/JIT fixtures pass for file/memory/HTTP,
confirmed steps/resume/pause, startup and async/blocking child cancellation after
body receipt, paused completion and early/runtime-error joined cleanup. C++98
host linkage and native mailbox/failure fixtures pass. All stages remain open.

v0.713 adds a bounded C89 native playback mailbox with latest-intent coalescing,
terminal STOP precedence, mutex-protected scalar/text copies and explicit lifetime.
Four OS readers, quota/output validation and native allocation/mutex rollback pass;
live mailbox handles prevent premature SDK shutdown. All stages remain open.

v0.712 delivers a standalone annotated C89/AS/SWF playback tutorial with native/
cross-build instructions and an argv-only adapter. Owner commands remain polled
while slow timer-free movies await the next or final frame; real interpreter/JIT
fixtures stop both waits without waiting for the ten-second authored period.
All nine stages remain open.

Final v0.710–v0.712 verification: 56 selected native loading/control checks plus
13 additional audio/timer/redraw/final-frame/build-info regressions pass. Shared
host configurations pass 26 socket-TLS, 24 curl, 18 no-provider and 12 codec-
disabled checks. The changed C playback/player/transfer/audio modules pass UBSan
with interpreter/JIT and the standalone tutorial; VM/SDL/transport dependencies
are not instrumented. Audio-enabled player and the C playback tutorial cross-link
for Linux ARM64, Linux RISC-V64 and Windows x86-64, without foreign execution.
Source links, per-line tutorial comments/API tables, catalog ordering and
reproducible SWF/linked bytes pass. Native device/foreign OS/reference acceptance
and all nine roadmap stages remain open.

Русский: проверены 69 выбранных native-сценариев, четыре конфигурации shared-хоста,
C-only UBSan и кросс-линковка трёх CPU/OS. Нативного запуска на других ОС,
прослушивания/аппаратной приёмки и завершения пункта 9 это не подтверждает.

v0.711 integrates owner-thread pause/step/resume/stop with FILE/MEMORY/URL
runners, background loading, paused redraw/timers and suspended native PCM.
Four focused native checks pass, including real interpreter/JIT SWF fixtures,
HTTP completion/cancellation and host ABI. All nine stages remain open.

v0.710 adds an allocation-free C89 playback-command policy and separate host
descriptor: pause, resume, coalesced single tick and terminal stop. This is not
full VM debugger suspension; all nine stages remain open.

v0.709 upgrades C and executable AS/SWF tutorials with scalar content snapshots,
exact image/SWF counters and deferred-publication/version checks for URL/memory
startup. All nine stages remain open.

Final v0.707–v0.709 series: the 52-check native loading/query set and a broader,
overlapping 108-check MovieClip/SWF import/timeline/document/sound set pass.
Shared configurations pass 22 socket-TLS, 20 curl, 14 no-provider and eight
codec-disabled checks. Six UBSan fixtures instrument the changed MovieClip,
loading, content and SWF-timeline implementations; separate C player/transfer/audio
UBSan fixtures cover interpreter/JIT and URL/embedded startup. Other VM, codec,
SDL and transport dependencies are uninstrumented in those sanitizer runs.
Audio-enabled player and both image C examples cross-link for Linux ARM64,
Linux RISC-V64 and Windows x86-64 without execution. Source links, individual API
example tables, catalog checks and deterministic assets pass. All stages remain
open; full reference/API, progressive/internal-symbol and native OS/device
acceptance is outstanding.

v0.708 adds versioned getBytesLoaded/getBytesTotal/getSWFVersion methods backed
by committed import records. CWS totals use canonical bytes; no progressive or
internal-symbol accounting is claimed. All nine stages remain open.

v0.707 adds C89 committed-content snapshots: format, own SWF version, encoded/
decoded counts and original image dimensions. Atomic import/unload, duplicate and
serialized ownership are retained. All nine stages remain open.

v0.706 upgrades executable C/AS/SWF tutorials and local/HTTP/HTTPS fixtures with
named resizing, readonly origins and image redirects into another source directory.
All stages remain open.

Final v0.704–v0.706 series: 48 selected native checks pass; shared socket-TLS
18, shared curl 16, no-provider 10 and codec-disabled five pass. UBSan passes for
MovieClip geometry/dimension/location implementation tests, and separately for
C player/transfer/audio with interpreter/JIT and URL/embedded examples. Other VM,
codec, SDL and transport dependencies are uninstrumented in those checks.
Audio-enabled player and both image C tutorials cross-link for Linux ARM64,
Linux RISC-V64 and Windows x86-64; foreign executables are not run. Generated
assets are deterministic; source links and tutorial/catalog checks pass.
All nine stages remain open; full SWF/API, native OS/device and reference-player
acceptance remains outstanding.

v0.705 exposes readonly _url through named and numeric access, retaining the
loader source across unload/detachment/serialization. All stages remain open.

v0.704 connects named _width/_height accessors to intrinsic AVM1 geometry and
resize operations, including stable graph callback identities. All stages open.

v0.703 makes upstream image/vendor dialects target-private and configures Cairo
integer sizes for cross builds, including Windows LLP64/MinGW CRT declarations.
Final series: 40 selected native tests pass; shared socket-TLS 15 and shared curl
13 pass. Earlier same-series no-provider seven and codec-disabled two pass.
C-only player/transfer/audio UBSan covers interpreter/JIT and URL/embedded hosts;
VM/codecs/SDL/transport are not instrumented. Audio-enabled player and both C
examples cross-link for Linux ARM64/RISC-V64 and Windows x86-64 without execution.
All nine stages remain open; this does not certify complete loader/API conformance.

v0.702 adds executable core C89 and standalone player C/AS/SWF image tutorials,
with reproducible PNG/GIF/JPEG assets, URL/linked-memory startup, API/ownership
manuals, indexed documentation and native/cross build instructions. All stages open.

v0.701 connects child image bytes to the existing owner queue for local/network,
waiting/async and socket/curl transport. Actual image SWFs check identity, dimensions,
unload/SWF reload, malformed input and explicit-trust TLS. Startup/root still
requires SWF; all nine stages remain open.

v0.700 adds atomic standalone PNG/static GIF89a/JPEG content replacement and
signature-based SWF/image dispatch in the C API. Straight alpha, GC, detached
provenance, SWF baseline/progressive profiles and C++ allocation rollback are
verified. Codecs use the bundled static image profile. [EN](IMAGE_LOADING.md) /
[RU](IMAGE_LOADING.ru.md). All nine stages remain open.

v0.699 preserves direct object destinations in global movie/form calls and receiver
methods, without target stringification or path round-trips. Per-run bindings
share the existing bounded queue; URL coercion and detached/reused destinations
are checked in actual local/HTTP/async SWFs with negative controls. Final series:
46 selected native checks pass with SDL audio enabled; shared native-TLS 20,
shared curl 18 and no-provider nine pass. Shared test discovery was corrected;
TLS fixtures explicitly select their CA, and async root forms wait for completion.
C-only player/transfer/audio UBSan passes interpreter/JIT, root TLS and C tutorial
fixtures with uninstrumented VM/SDL/transport dependencies. Three audio-enabled
foreign player/tutorial builds cross-link for Linux ARM64/RISC-V64 and Windows
x86_64 without target execution. All nine stages remain open.

v0.698 adds level-zero loadMovieNum/unloadMovieNum facades over the shared queue,
AVM1 numeric coercion and new-root provenance. Unsupported levels and POST fail
before transport; local/HTTP and negative-control SWFs cover interpreter/JIT.
No multi-level stack is claimed; all nine stages remain open.

v0.697 adds standalone root/audio SWF and linked-array variants to the annotated
C/AS tutorial. Four C-host cases cover child/root and URL/embedded startup with
explicit provenance. Native audio CLI setup and dummy-device limits are documented.
All nine stages remain open.

v0.696 enables root/_level0 content clearing without ending context/window.
The owner detaches sound delivery and joins/releases native PCM before network
cancellation, preserving cumulative counters. Actual root SWFs test retained
handlers, obsolete frames, subtree cancellation, relative forms and looping PCM
with dummy SDL audio enabled. No synthetic unload events; all stages remain open.

v0.695 extends that C tutorial with immutable SWF bytes linked into the executable.
Explicit network provenance survives unload and resolves relative form reloads;
URL/memory fixtures prove memory startup does not fetch a startup SWF. No temporary
movie file or binary strlen. Final series: 35 selected native tests, 11 shared
native-TLS, nine shared curl and three no-provider tests pass. C-only player/transfer
UBSan passes interpreter/JIT and URL/linked-memory tutorial fixtures; VM/SDL/transport
libraries are not instrumented. Player/tutorial cross-link for Linux ARM64/RISC-V64
and Windows x86_64 without target execution. All nine stages remain open.

v0.694 adds an independent annotated C89/AS/SWF unload tutorial with its own
bytecode generator, delayed loopback server, native/cross instructions and RU/EN
ownership/API guides. Its real C-host fixture verifies cancelled subtree I/O,
retained peer delivery and reload. All stages remain open.

v0.693 rejects active/queued destinations under removed ancestors, keeping exact
identity when the same script path is reused. It also drains onData-generated
unloads before the next fetch in synchronous/waiting/async modes. Removed-subtree,
retained-reference and callback SWFs extend the interpreter/JIT fixture.
All nine stages remain open.

v0.692 adds deferred child unloadMovie in the player, before active-job polling.
Earlier subtree requests are cancelled/joined or dropped; later reloads and unrelated
FIFO requests survive. Local forms work with zero remaining fetches. Actual HTTP
and local SWFs pass interpreter/JIT including negative control. Root unload and
full unload events remain unsupported; all nine stages remain open.

v0.691 adds atomic C API content clearing and native ancestry queries, retaining
instance identity, placement, handlers and source bases. Rendering, GC and
allocation-failure regressions pass; the C89 MovieClip tutorial demonstrates
safe-point use. 92 selected native checks and three shared-library checks pass.
Player and C89 tutorial cross-link for Linux ARM64/RISC-V64 and Windows x86_64,
without target execution. This does not install script unloadMovie or cancel host transfers.
[EN](MOVIE_UNLOAD.md) / [RU](MOVIE_UNLOAD.ru.md). All nine stages remain open.

v0.690 adds --http-ca-file without implicit redirects and verifies complete
HTTP/HTTPS redirected playback with explicit trust on both native and libcurl
providers, including single HTTPS sources without redirects and missing-trust errors.
C/IDE uses the same borrowed ca_file policy. 99 selected native checks pass with
one build-artifact execute-permission rerun, then four final focused redirect checks.
Shared native-TLS: 24; shared curl: 20 plus final redirect reruns; three no-provider checks. C-only UBSan passes
HTTP/TLS/player/URI/redirect/clock probes with ordinary VM/SDL/socket/TLS libraries.
Three foreign player/example builds and Windows DLL C/C++98 consumers cross-link,
without target execution. All stages remain open.

v0.689 exposes explicit network-policy source/playlist C APIs and --redirects.
Actual SWFs verify final root/child/form bases, memory origins, waiting/async and
interpreter/JIT execution, with native TLS and an annotated standalone C/server
example. Four logical resources traverse eight HTTP requests. All stages remain open.

v0.688 adds the matching libcurl policy through the shared resolver/deadline loop,
plus verified multi-hop TLS, HTTP upgrade, blocked downgrade and bad later-certificate
regressions for both providers. Old callbacks retain no-redirect semantics.

v0.687 adds explicit bounded native GET redirects with one total deadline,
final-only body/provenance, strict destinations and per-hop TLS trust. Old fetch
contracts remain redirect-free. Raw HTTP/URL/framing checks pass.
[EN](HTTP_REDIRECTS.md) / [RU](HTTP_REDIRECTS.ru.md). All stages remain open.

v0.686 exposes the same monitor through a default-provider playlist C API and
strict --load-timeout/--load-progress CLI translation. No scalar ABI change or
second network/scheduling implementation. Tests cover malformed options before
startup, zero disabling, waiting/async delivery, timeout and two-source playlists.
18 monitored HTTP/HTTPS scenarios include memory roots and whole-root replacement.
Series validation: 95 selected native tests plus 17 focused checks after the stop-latch
correction; 20 shared native-TLS, 16 shared curl, 3 no-provider checks. Player/transfer
C-only UBSan passes monitor/CLI/jobs; dependencies are not instrumented. Three foreign
player/example builds and Windows DLL C/C++98 consumers link, without execution.
[EN](LOAD_MONITOR.md) / [RU](LOAD_MONITOR.ru.md). All stages remain open.

v0.685 adds owner-thread source load monitoring, startup stop before I/O,
per-network-job deadlines and explicit result/cancellation notices. HTTP/HTTPS
fixtures cover16 scenarios each; observer state never enters a worker. Annotated
C host and RU/EN guides: [EN](LOAD_MONITOR.md) / [RU](LOAD_MONITOR.ru.md).
This is host integration, not full Flash loader lifecycle; all stages remain open.

v0.684 adds additive cooperative total job deadlines with timeout-specific terminal
views, retained partial progress, immutable publication and joined lifetime. Native
job fixtures and C++98 ABI checks pass; DNS/hard preemption remain outside the contract.

v0.683 adds opt-in asynchronous network child/form delivery: one pinned FIFO
request, continuing frames/timers/input/GC, owner-thread import/onData and joined
cancellation before context teardown. 91 selected native tests pass, including
HTTP/HTTPS interpreter/JIT and 1Hz completion latency with negative controls.
12 shared-libcurl and 16 shared native-TLS tests pass; player/transfer C-only
UBSan passes the async HTTP fixture. Dependencies are not instrumented.
Player/example cross-builds link for Linux ARM64/RISC-V64 and Windows x86_64;
Windows DLL C/C++98 consumers link without target execution. Full load lifecycle,
progressive decoding, DNS interruption and foreign runtime acceptance remain open.
[EN](BACKGROUND_TRANSFER.md) / [RU](BACKGROUND_TRANSFER.ru.md). All nine stages remain open.

v0.667 adds a foreground event-loop GC safe point independent of fired timers.
[Contract and regression](PLAYER_EVENT_GC.md) / [Контракт и проверка](PLAYER_EVENT_GC.ru.md).
v0.668 validates embedded player argument vectors before allocation/startup.
v0.669 adds device-free version/build metadata and a C/C++98 host query.
[Metadata contract](PLAYER_BUILD_INFO.md) / [Контракт метаданных](PLAYER_BUILD_INFO.ru.md).
Current v0.669 validation: 62 selected native tests pass; full players cross-link
for ARM64/RISC-V64 Linux and x86-64 Windows without target execution.
This is bounded progress on stages 3/7/9; all nine stages remain open.

v0.670 replaces global SDL shutdown with balanced per-window VIDEO/TIMER references.
Host windows and unrelated subsystem references survive player cleanup and video failure.
[Embedding ownership EN](PLAYER_HOST_OWNERSHIP.md) / [RU](PLAYER_HOST_OWNERSHIP.ru.md).

v0.671 preserves embedding-host touch/mouse SDL hint values and priorities.

v0.672 adds a direct C89/C++98 configured runner, complete pre-start configuration
validation, the optional static host target and an annotated embedding example.
[Structured runner EN](PLAYER_CONFIGURATION.md) / [RU](PLAYER_CONFIGURATION.ru.md).
The muted-stream tutorial CTest now supplies its required SWF argument.
Current v0.672 validation: 65 selected native checks pass. Full players and the
new configured-host example cross-link for ARM64/RISC-V64 Linux and x86-64
Windows; target executables are not run. This does not close stage 9.

v0.673 adds exact borrowed memory playback and a linked-resource C89 example.
[Sources EN](PLAYER_SOURCES.md) / [RU](PLAYER_SOURCES.ru.md). All nine stages remain open.

v0.674 adds bounded host-provider input and deferred GetURL2 root replacement.
Child-clip/progressive loading and full loadMovie semantics remain open.

v0.675 adds optional HTTP(S), CLI root-load budgets and static/shared resource modules.
Full child-clip loading, asynchronous/progressive transport and actual foreign-platform
acceptance remain open.

v0.676 adds same-context child replacement, form variables/onData, per-movie origins,
executable-base memory loading and a pinned safe-point FIFO. [Contracts/tests EN](PLAYER_SOURCES.md) / [RU](PLAYER_SOURCES.ru.md).
Validation: 75 selected native tests and 8 shared-host tests passed, with subsequent
focused checks after source-origin changes. Static cross-builds cover Linux ARM64,
Linux RISC-V64 and Windows x86_64; Windows DLL exports and C/C++98 consumers link.
Foreign executables were not run. Full historical loading and actual foreign runtime
acceptance remain open; none of the nine stages is declared complete.

v0.677 adds optional native POSIX/Winsock HTTP GET, bounded URI resolution,
strict response framing and monotonic TCP/I/O deadlines without libcurl.
Six native tests cover URI/raw TCP and interpreter/JIT root/child/form loading.
Linux ARM64/RISC-V64 and Windows x86_64 player cross-builds link; not executed.
[EN](SOCKET_TRANSPORT.md) / [RU](SOCKET_TRANSPORT.ru.md). TLS and async DNS remain open.

v0.678 adds optional verified OpenSSL TLS over the same native TCP deadline adapter,
with per-request trust configuration, DNS/IP identity and SNI. Seven native tests
pass, including TLS1.2/1.3, bad trust/name/expiry, EOF/deadlines, interpreter/JIT
HTTPS child/forms and the C host. The transport probe passes UBSan; OpenSSL/VM
are not instrumented by that probe. Foreign TLS SDK/runtime acceptance remains open.

v0.679 exposes compiled network capabilities through C89/C++98 and --version
without changing the build-info layout or opening network/display resources.
Final native selection: 78 tests covered, with focused reruns after catalog/helper
fixes; shared TLS 12, libcurl 9, no-provider metadata/ABI 2. Generated URI tests pass
ASan/UBSan (LSan disabled: unavailable /proc task inventory); transport probes pass
UBSan. Three plain HTTP cross targets and Windows DLL/C/C++98 consumers link;
foreign execution/TLS SDK acceptance and full stage 9 remain open.

v0.680 adds cooperative native HTTP/TLS and libcurl cancellation through additive
C89 APIs, preserving options/loader layouts. Native waits check at 50ms intervals;
DNS/CA/user code is not forcibly interrupted. Silent/drip HTTP/TLS and libcurl
cancellation probes pass. [EN](BACKGROUND_TRANSFER.md) / [RU](BACKGROUND_TRANSFER.ru.md).

v0.681 adds joined background byte-transfer jobs, synchronized polling/cancellation,
immutable result views, bounded buffers/handle quota and SDK shutdown protection.
Owner/worker, errors, cancellation, quota and cleanup fixtures pass; VM objects
never cross the worker boundary. [EN](BACKGROUND_TRANSFER.md) / [RU](BACKGROUND_TRANSFER.ru.md).

v0.682 integrates explicit --background-loads and C background runners with native
event/audio pumping, joined cancellation on close and owner-thread import/GC.
Timeline/timers still wait at the resource safe point; startup/root transitions
have no active progress view. 86 native, 11 shared TLS, 9 shared curl and 3 no-provider
checks pass; transfer-only UBSan passes with SDK libraries uninstrumented.
Three plain HTTP cross targets and Windows DLL/C/C++98 fixtures link, not run.
[EN](BACKGROUND_TRANSFER.md) / [RU](BACKGROUND_TRANSFER.ru.md). All stages remain open.

| Stage / Этап | Existing work / Основа | Remaining acceptance / Что не закрыто |
| --- | --- | --- |
| 1: objects/types | Object/Array/String, property cases and ownership reports in [COMPATIBILITY_STATUS.md](COMPATIBILITY_STATUS.md) | Complete version-specific object/string semantics and reference comparisons; old roadmap entries do not imply every listed feature is wholly absent |
| 2: AVM1 | All 100 bounded dispatch-catalog entries have source references; [AVM1_DISPATCH_INVENTORY.md](AVM1_DISPATCH_INVENTORY.md) | Full operand, scope, exception, conversion and version semantics against original behavior; source branches alone are insufficient |
| 3: scene/timeline | MovieClip/resource/control/geometry reports; root sound-block framing | Full scene/resource behavior, automatic audio clock, sprite sound integration and real movie/reference coverage |
| 4: graphics/media | Vector/text adapters, explicit audio voices/mixing, PCM streams, native multimedia bridges | Complete effects/text/codecs/video and device playback/capture. v0.576 adds encoded capture and MP3-prefix inspection, not MPEG decoding |
| 5: standard APIs | Implemented standard objects and bridge contracts in the compatibility register | Complete method/object inventory and missing networking, loading, XML, event/input/system behavior |
| 6: extensions | Channel/thread/graph-transport and ownership tests documented by subsystem | Full distributed failure, shared-resource, synchronization and extension-ABI acceptance |
| 7: GC/debugger | Root/OOM tests, fault policies and source-aware debugger API | Coverage across every new subsystem; concurrency, finalization, deadlock and intercepted-channel diagnostics |
| 8: JIT | Native Linux subset; foreign-ISA encoders and Windows/macOS protocol fixtures | Full advertised AVM1/native equivalence and actual runtime acceptance on each CPU/OS; fixtures are not foreign-platform runs |
| 9: platforms/release | Linux build/test infrastructure and target-specific adapters | Actual Windows/macOS/Android build/run/device/endurance acceptance, complete compatibility matrix and reproducible release gates |




## v0.666 legacy cursor and muted streams / Старый курсор и muted streams

The player installs the SWF5 cursor subset while retaining SWF6 listener gates.
An additive exact-source import flag permits validated root stream controls only
when explicitly muted; the strict API and sprite-stream rejection remain.
File reading detects measured-length changes/EOF failures, and numeric CLI options
avoid process-global locale conversion. A C89/GC tutorial and SWF8 fixture are
included. [Contracts/tests EN](PLAYER_PREFLIGHT.md) / [RU](PLAYER_PREFLIGHT.ru.md).
No streaming output decoder/clock or complete physical/foreign acceptance is claimed.
All nine stages remain open. Все девять стадий остаются открытыми.

## v0.661 queued input fairness / Очередь и границы ввода

A real queued-wheel regression corrects v0.653's double viewport conversion.
SDL event batches yield after 64 records; subsequent key/close records survive.
Modern touch records honor window identity, while leave/focus/background restore
host cursor visibility without overwriting the VM request. SWF6–8 storm fixtures
exercise the actual SDL queue, copied FIFO and interpreter/native JIT callbacks.
[Contract EN](PLAYER_EVENT_FAIRNESS.md) / [RU](PLAYER_EVENT_FAIRNESS.ru.md).
No physical/foreign runtime or complete stage acceptance is claimed.
All nine stages remain open. Все девять стадий остаются открытыми.

## v0.656 player wheel and cursor / Колесо и курсор проигрывателя

The SDL player routes vertical/precise wheel entries through its bounded shared
input FIFO, maps modern event positions through the logical viewport and applies
script cursor requests at owner-thread safe points. Native cursor visibility is
restored across playlist teardown. SWF6–8 interpreter/JIT fixtures use real SDL
dummy queues and wrong-value controls; a stopped SWF8 tutorial is included.
[Contract EN](PLAYER_WHEEL_CURSOR.md) / [RU](PLAYER_WHEEL_CURSOR.ru.md).
This is not physical/foreign-platform acceptance. All nine stages remain open.
Все девять стадий остаются открытыми.

## v0.651 player lifecycle recovery / Восстановление проигрывателя

The optional SDL player owns the last packed frame, restores lost device textures,
defers background recovery and redraws on foreground without an extra VM tick.
Suspended transitions and foreign-window key/mouse/window events are discarded;
repaint failures propagate separately from close. The final-frame audio hold now
honors background pause. [Contract and checks EN](PLAYER_RECOVERY.md) /
[RU](PLAYER_RECOVERY.ru.md). Native Linux dummy-video and protocol checks do not
certify a physical GPU, audible device, Activity or foreign native runtime.
All nine stages remain open. Все девять стадий остаются открытыми.

## v0.646 cross SDK fixes and readable tools / Cross SDK и читаемые исходники

Strict Windows/Clang builds scope the upstream zlib ULLONG_MAX diagnostic to
zlib alone; first-party C89/C++98 checks remain enabled. SAPI final-recognition
masks use typed shifts with identical SDK bits, avoiding C++11 literal macros.
Linux ARM64/RISC-V64 and Windows x64 minimal cores and real C89 embedding clients
cross-build/link. Four syntax frontends, FIR/source/optimizer, SWL and the combined
C IDE host also cross-build for all three targets. No foreign execution is claimed.
[Build guide EN](../../BUILDING.md) / [RU](../../BUILDING.ru.md),
[evidence EN](../../toolchains/VALIDATION.md) / [RU](../../toolchains/VALIDATION.ru.md).

Six authoritative ATGs have aligned final C blocks and explicit EBNF nesting;
45 first-party C99 source/header files are expanded and component formatting
policies retained. Tutorials and catalogs document API ownership and numeric/
syntax boundaries. Все девять стадий остаются открытыми.

## v0.645 bounded tests and UTF-8 evidence / Таймаут и UTF-8

The native runner records a positive default per-test timeout. A real sleeping
CTest child fails within its configured one-second limit. Alternate XML encodings
cannot bypass forbidden declaration checks. No runtime VM semantic change or new
foreign-platform acceptance is claimed. [EN](RELEASE_EVIDENCE.md) /
[RU](RELEASE_EVIDENCE.ru.md). Все девять стадий остаются открытыми.

## v0.644 reproducible evidence bundles / Переносимые доказательства

Validated staged snapshots produce deterministic, manifest-indexed evidence ZIPs.
Five export tests cover relocation, tampering and atomic publication. A real
miniature C89/C++98/CMake/CTest pipeline covers report/matrix/export integration;
it is not a complete VM run. [EN](RELEASE_EVIDENCE.md) / [RU](RELEASE_EVIDENCE.ru.md).
Foreign runtime/device acceptance and all nine roadmap stages remain open.

## v0.643 portable native matrix / Сводная матрица платформ

A separate audit recomputes relocated CTest artifacts and checks source revision,
clean state, native ABI and command provenance. Missing required OS/profile cells
block the matrix; exact skips remain limitations. Eleven fixture tests pass.
[EN](RELEASE_EVIDENCE.md) / [RU](RELEASE_EVIDENCE.ru.md).
This is not full stage-9 certification. Все девять стадий остаются открытыми.

## v0.642 active native target / Нативная цель и ABI

The native reporter validates active CMake OS/architecture, non-cross mode and
C/C++ pointer ABI against the executing process. Stale metadata is ignored.
Seven policy tests and the real Linux x86-64 configuration pass; foreign OS
fixture branches do not certify those platforms. [EN](RELEASE_EVIDENCE.md) /
[RU](RELEASE_EVIDENCE.ru.md). Все девять стадий остаются открытыми.

## v0.641 complete CTest execution evidence / Полнота CTest

Native platform reports now bind exact configured names to fresh JUnit outcomes,
record acknowledged skips separately and reject disabled/incomplete executions.
Real CTest filter/skip/disable fixtures and source-identity regressions pass.
[EN](RELEASE_EVIDENCE.md) / [RU](RELEASE_EVIDENCE.ru.md).
This advances release tooling, not full VM or foreign-platform acceptance.
Все девять стадий остаются открытыми.

## v0.640 canonical root sound frames / Канонические звуковые кадры

One bounded root-stream index binds captured lists to their canonical frame:
order, membership and completeness are validated before host callbacks, including
missing tables/records on a source-bound root. Empty canonical frames need no
callback. Frame action replacement retains a valid imported sound list and stages
a missing reentry state field before publishing the frame. [EN](SOUND_SNAPSHOT.md#canonical-frame-binding-v0640) /
[RU](SOUND_SNAPSHOT.ru.md#канонический-кадр-v0640).

Five new regressions cover membership/omissions/callback policy, index work/cap/
malformed tails, transactional index OOM, SWF6–8 queue/serialization/source destruction/
traced GC/native JIT/repair-retry, and action replacement/OOM publication. The
updated sound snapshot work/invalid tests retain their earlier coverage with the
new canonical checks. Exact v0.639 sound source reproduces wrong-frame/duplicate
acceptance; a source-linked pre-staged-entry control reproduces frame publication
before a later OOM. The current implementations reject/preserve as specified.
All nine stages remain open. Все девять стадий открыты.

Validation for v0.640: all 141 selected minimal Linux Release CTests and all 141
matching whole-core heap/global ASan + UBSan CTests passed in complete final runs.
The sanitizer run is serial, with stack-ASan and LeakSanitizer disabled in this
traced host. Final test binaries were linked against one final core; stale/mixed
preparation builds were rejected, and executable permissions were repaired before
final runs. Fourteen new regression executables cover this series. The C89 mixer
tutorial oracle checks --demo/--timeline-demo/--convert-demo and exact native PCM
files; constants/header, source-link, deterministic documentation, debugger-example
and 26 existing platform-tool tests passed. Exact old-source controls reproduce
ordinal ordering, uncharged work, canonical mismatch and late frame-publication OOM.
These are software/minimal-backend results: ALSA unsupported-stub checks are not
physical audio, and no AGG/Cairo/SDL, original-Player, device or foreign-OS acceptance
is claimed. All nine roadmap stages remain open.

Проверки v0.640: успешны все 141 выбранных Linux Release CTest и все 141 matching
whole-core heap/global ASan + UBSan CTest полными финальными прогонами. Sanitizer run
последовательный; stack-ASan/LeakSanitizer отключены в traced-среде. Бинарники
согласованы с окончательным ядром; устаревшие/смешанные сборки не считались успешными,
права исполнения исправлены до финального прогона. Добавлено 14 regression programs.
C89 mixer tutorial проверяет три режима и точные PCM-файлы. Header/source/doc checks
и 26 прежних platform-tool tests успешны. Контрольные старые исходники воспроизводят
исправленные ошибки порядка, бюджета, canonical binding и публикации кадра до OOM.
Minimal backend/ALSA stubs не подтверждают физический звук, AGG/Cairo/SDL, оригинальный
Player, устройства или другие ОС. Все девять стадий остаются открытыми.

## v0.639 sound metadata work / Бюджет звуковых metadata

Six read-only definition/event/envelope queries account root tags, headers and
points under shared work, including both definition-at scans and event-to-ID
resolution. Error outputs remain untouched and the queries allocate nothing.
[EN](SOUND_SNAPSHOT.md#metadata-work-v0639) /
[RU](SOUND_SNAPSHOT.ru.md#бюджет-метаданных-v0639).

Validation: twelve selected minimal Linux Release CTests passed. Four new exact
SWF1–8/nested/saturated/output/recovery/allocation-free/worker tests include six
actual OS workers; previous event edges/submission/OOM, root delivery/transport
and snapshot work/stop tests passed. Exact v0.638 dictionary source fails the new
work-counter assertion. All nine stages remain open; decoder/output aggregate
work, canonical frame binding, reference/device/foreign-OS acceptance remain open.
Все девять стадий открыты; эти результаты не подтверждают прослушивание звука.

## v0.638 shared root sound work / Общий бюджет звуковой доставки

Full-source comparison, ordinal capture and host dispatch share the AVM1/tick
counter and periodic worker stops. [EN](SOUND_SNAPSHOT.md#delivery-work-v0638) /
[RU](SOUND_SNAPSHOT.ru.md#бюджет-доставки-v0638). Earlier host effects and consumed
frame entries remain committed; native callback wall time is not preempted.

Validation: ten selected minimal Linux Release CTests passed. New exact work,
nested/saturated and two actual OS-worker cases passed with earlier snapshot,
OOM, lifecycle/validation/transport and C89 tutorial checks. Exact v0.637 source
fails the new work-counter assertion; current source passes. Metadata lookup and
canonical frame binding remain separate work. All nine stages remain open.
Все девять стадий открыты; audio devices/original Player/foreign OS не проверены.

## v0.637 root sound ordinal capture / Снимки звуковых событий

Dense bounded scalar sound snapshots are captured by ordinal and completely
validated before first host delivery. [EN](SOUND_SNAPSHOT.md) /
[RU](SOUND_SNAPSHOT.ru.md). Host callback editing remains forbidden; robustness
fixtures are private tests. Canonical membership and broader work remain open.

Validation: eight selected minimal Linux Release CTests passed, including three
new snapshot/invalid/OOM tests, old sound lifecycle/validation/transport/import-OOM
and C89 tutorial checks. Exact v0.636 sound source fails the shuffled-ordinal
regression; current source passes. All nine stages remain open.
Все девять стадий открыты; физический звук и другие ОС не проверены.

## v0.636 scene graph boundaries / Границы графа сцены

Same-context state/table checks precede legacy slot migration; a bounded migration
precharges copying before publication and preserves output on error. Normalized
tables keep the caller's existing check timing. Reconstruction rejects every local
invalid/duplicate/self/foreign child identity before old-child removal. Scene-input
preparation preserves helper work errors. [EN](SCENE_CONTROLS.md#graph-validation-and-legacy-depths-v0636) /
[RU](SCENE_CONTROLS.ru.md#граф-сцены-и-старые-глубины-v0636).

Four new regressions cover ownership, exact/nested legacy/input work boundaries,
allocation sweeps/retry/retained allocation counts, and SWF6–8 serialization/channel
transfer with source destruction, traced strings, interpreter/native JIT and
receiver-owned native callback registration. Exact v0.635 MovieClip source fails
the new foreign-table boundary assertion; the current source passes. Constructor
ordering at seek remains unchanged pending original-Player reference evidence.
All nine stages remain open. Все девять стадий остаются открытыми.

Validation for v0.636: all 87 selected minimal Linux Release CTests passed; all
87 matching whole-core heap/global ASan + UBSan CTests passed in a final serial
run after relinking against the final core. Stack-ASan and LeakSanitizer are
disabled because this traced host cannot provide reliable stack/process metadata.
Two sanitizer executables needed execute-bit repairs; the final complete run
passed. The earlier unmodified-stack/LSan run is not counted as passing evidence.
The new nine series regressions, old scene/input/timer/frame/initializer/JIT tests,
C89 tutorial, strict C89 constants-header probe and documentation checks passed.
A C89 probe reported native JIT availability=1; tests assert actual native entries.
These results do not include AGG/Cairo pixel, SDL/device, original-Player or foreign
OS acceptance and close no roadmap stage.

Проверки v0.636: успешно 87 выбранных Linux Release CTest и 87 соответствующих
whole-core heap/global ASan + UBSan CTest последовательным финальным прогоном.
Stack-ASan и LeakSanitizer отключены из-за ограничений traced-среды. Два sanitizer
бинарника потребовали восстановления executable-бита; финальный полный прогон
успешен. Старый прогон с полной stack/LSan-проверкой не считается успешным.
Проверены девять новых regressions серии, старые scene/input/timer/frame/initializer/
JIT tests, C89 tutorial, строгий C89 header probe и документация. JIT доступен,
тесты проверяют native entries. AGG/Cairo/SDL, физические устройства, оригинальный
Player и другие ОС этим прогоном не подтверждены. Все девять стадий открыты.

## v0.635 shared scene work / Общий бюджет сцены

Incremental controls and seek/loop composition now share AVM1 work with nested
constructors. Copy/apply, authored-frame gaps, old-child snapshots and desired
placements are charged; numeric zero still polls worker stops. [EN](SCENE_CONTROLS.md#scene-work-v0635) /
[RU](SCENE_CONTROLS.ru.md#бюджет-сцены-v0635).

Validation: 13 selected minimal Linux Release CTests passed, including exact
budget/nesting/saturation/gap boundaries, two real workers and the previous
snapshot/OOM/scene/JIT/frame/initializer/tutorial checks. Exact v0.634 scene source
fails the new work-counter assertion; current source passes. No foreign-platform,
original-Player or stage-completion claim follows. Все девять стадий открыты.

## v0.634 captured scene controls / Снимки управляющих записей

Per-frame dense, bounded control snapshots precede constructors; scalar/list
mutation cannot replace the current batch. [EN](SCENE_CONTROLS.md) /
[RU](SCENE_CONTROLS.ru.md). Shared resources and prior side effects remain explicit.

Validation: 11 selected minimal Linux Release CTests passed, including three new
snapshot/invalid/OOM regressions, existing scene/JIT/frame/initializer tests and
the C89 tick tutorial. One emitted test lacked execution permission; its permission
was repaired before the successful rerun. Exact v0.633 scene source linked with
the new mutation test fails its expected-depth assertion; current source passes.
No original-Player, physical-device or foreign-platform run is claimed. All nine
stages remain open. Все девять стадий открыты.

## v0.633 stable platform source evidence / Стабильность исходников

Native validation now compares component source/index/HEAD endpoints before an
overall pass. Dirty-to-dirty changes can no longer reuse a successful tool result
as evidence for unchanged sources. [EN](PLATFORM_VALIDATION.md#source-stability-v0633) /
[RU](PLATFORM_VALIDATION.ru.md#стабильность-исходников-v0633).

Validation: seven isolated source-evidence tests, ten existing encoder/compiler
metadata tests and nine ELF evidence tests passed on Linux/Python 3. No CMake/CTest
or native VM run is claimed for this revision: CMake is unavailable in this host.
All nine stages remain open; endpoint hashing does not prove reproducible builds
or Windows/macOS/Android execution. Все девять стадий открыты.

## v0.632 pending initializer work / Бюджет оставшейся инициализации

Completed library initializers no longer copy/charge their consumed prefix on every
tick. Remaining ordinals use bounded, owner-checked snapshots and keep the existing
at-most-once attempt cursor, reentry guard and fresh progress after graph transport.
[EN](INITIALIZER_PROGRESS.md) / [RU](INITIALIZER_PROGRESS.ru.md).

Это продвижение стадий 2/3/6/7/8/9; общий lifecycle, original Player timing, все CPU/JIT
и физическая приёмка платформ остаются открытыми вместе со всеми стадиями 1–9.

Validation v0.632: 204 selected Linux Release CTests (AGG/static dependencies) and
147 minimal whole-core ASan/UBSan CTests passed. The latter run is serial, without
raster and with LeakSanitizer disabled. Six new regressions plus earlier init/action,
frame/graph/input/timer/JIT tests passed. Strict C89 tutorial linked to sanitizer core
and printed six lines; documentation checks passed. A source-linked v0.631/current
control reproduces/fixes a completed table exhausting an empty tick's two-unit budget.
No full original-Player, SDL/device/foreign-OS or stage-completion claim follows.

## v0.631 bounded frame-action snapshots / Блоки действий кадра

Tick and ActionCall share dense ordinal snapshots instead of insertion-order iteration.
Direct native frame calls retain a single instruction/work budget across every block;
the public service includes operand coercion. Label lookup is charged and duplicate
labels choose the lowest stored index. GC-rooted frame calls retain real collection,
while tick keeps its native barrier and legacy single-code accounting.
[EN](FRAME_BLOCKS.md) / [RU](FRAME_BLOCKS.ru.md).

Продвижение стадий 2/3/6/7/8/9; полная совместимость SWF/Player, все CPU/JIT и
физическая приёмка платформ остаются открытыми вместе со всеми девятью стадиями.

Validation v0.631: 196 selected Linux Release CTests (AGG/static dependencies) and
139 minimal whole-core ASan/UBSan CTests passed. The sanitizer run is serial,
without a raster backend and with LeakSanitizer disabled. Nine new frame regressions,
existing ActionCall/missing-input/JIT dispatch and timeline/graph/input/timer tests,
strict C89 tutorial and documentation checks passed. Source-linked v0.630/current
comparison reproduces/fixes per-block budget resets; no original-Player, SDL/device
or foreign-OS acceptance follows from this evidence.

## v0.630 raster depth snapshots / Снимки raster-обхода

Raster traversal replaces quadratic insertion sorting with the shared heap sort,
without changing native instruction accounting. Local identity snapshots validate
owners/aliases/bounds before child getters; replacement and detachment are explicit.
The C89 tutorial demonstrates rendering without advancing timelines.
[EN](RASTER_DEPTH_ORDER.md) / [RU](RASTER_DEPTH_ORDER.ru.md).

Это продвижение стадий 3/4/6/7/9. Полная геометрия, общий raster budget, lifecycle,
оригинальный Player и физическая приёмка других платформ остаются открытыми.

Validation v0.630: 183 selected Linux Release CTests with AGG/static dependencies
and 125 minimal whole-core ASan/UBSan CTests passed. The latter run is serial,
with LeakSanitizer disabled and no raster backend; actual pixel assertions passed
in Release. Four new raster regressions, the large shared-sort oracle, strict C89
tutorial and documentation checks passed. Source-linked v0.629/current comparison
reproduces/fixes premature replacement delivery; it is not original-Player evidence.

## v0.629 native timeline depth snapshots / Глубины tick-обхода

Corrects a documented-but-unimplemented ordering promise: native child hash copies
previously iterated insertion order. Post-handler child snapshots now validate local
receiver identities and ownership, sort depths under the shared work budget, preserve
captured identities through replacements and recheck effective ancestor removal.
Input tree routes share the same heap sort with unchanged sorting units. Early private
container/state checks reject unsafe null/foreign edges. The C89 tick tutorial adds a
separate finite-budget shuffled-depth example. [EN](TIMELINE_DEPTH_ORDER.md) /
[RU](TIMELINE_DEPTH_ORDER.ru.md).

Это продвижение стадий 3/6/7/8/9, не whole-tree lifecycle scheduler или полная приёмка
original-player ordering. Все девять этапов открыты; физические устройства и чужие
платформы этой серией не сертифицируются.

Validation for v0.629: 172 selected Linux Release CTests (AGG/static dependencies,
including three runnable tutorials) and 112 minimal whole-core ASan/UBSan CTests
passed. Ten new core regressions cover sorting, identity, detachment, private edges,
local bounds, work/OOM and transferred AVM1/JIT ordering. The final sanitizer run
was serial, with LeakSanitizer disabled; the exhaustive clone-allocation test took
81.61 seconds within its explicit 180-second limit. Earlier mixed cached objects
were rebuilt, and load-induced parallel timeouts were not counted as passing.
The strict C89 example also ran against the instrumented VM and printed the verified
low/middle/high order. Source-linked old/new controls reproduced the v0.628 insertion
order defect. Source links, deterministic generation and annotated tutorial checks
passed. No SDL/device/foreign-OS rerun is claimed by these Linux core results.

Проверки v0.629: успешно 172 выбранных Release CTest и 112 whole-core ASan/UBSan
CTest, включая десять новых regressions. Финальный sanitizer run последовательный,
LeakSanitizer выключен; clone OOM matrix прошёл за 81,61 с при лимите 180 с.
Strict C89 example выполнен под санитайзерами. Старые cache objects пересобраны;
таймауты параллельного запуска не выданы за успешную приёмку. Все девять стадий
открыты; SDL/физические устройства/чужие ОС этой серией повторно не проверены.

## v0.628 retained KeyPress input / Коды и доставка KeyPress

Pure copied-snapshot translation now distinguishes SWF special selectors from
virtual Key codes and preserves existing English modifier mapping. A separate
bounded tree API delivers exact raw KeyPress records for source SWF6–8/host zero,
sharing topology/work/GC/stop/error policy. The SDL player selects from the queued
snapshot after successful KeyDown records/methods, with release/unmapped suppression,
repeat delivery, epoch checks and existing focus deduplication. The C89 tutorial
adds explicit Left 37→1, marker 168 and coalesced redraw.
[EN](CLIP_KEYPRESS.md) / [RU](CLIP_KEYPRESS.ru.md).

Это развитие стадий 3/5/6/7/8/9; DefineButton2 conditions, targeted interaction,
lifecycle, IME/layout, original-player order и physical/foreign-platform acceptance
не закрываются. Все девять этапов открыты.

Validation for v0.628: 286 selected Release CTests and 101 whole-core ASan/UBSan
CTests passed on Linux, including eight new core tests and three new frontend/player
checks. LeakSanitizer was disabled in this environment. The updated strict C89
example also ran under ASan/UBSan and produced Left selector 1, marker 168 and a
coalesced redraw request. A separately linked player with the v0.627 SDL adapter
rejected the same SWF8 KeyPress trace movie as expected. Documentation source links,
deterministic regeneration and debugger tutorial checks passed. SDL dummy/synthetic
input does not certify physical devices, foreign OS or full original-player order.

Проверки v0.628: успешно 286 выбранных Release CTest и 101 whole-core ASan/UBSan
CTest, плюс strict C89 example под санитайзерами. Контрольный player со старым
v0.627 SDL adapter ожидаемо отверг тот же SWF8 KeyPress movie. Leak detection был
выключен. Все девять этапов открыты; physical/foreign/reference acceptance не закрыта.

## v0.627 MovieClip input methods / Методы ввода клипов

A separate combined scene API delivers retained records and then optional MovieClip
onMouseMove/Down/Up and onKeyDown/Up methods. The retained-only API is unchanged.
Named methods are source SWF6–8/host version zero; source SWF5 still receives records.
One prepared topology/work/GC barrier covers both stages, with ancestry revalidation
between them. Method lookup preserves ordinary inherited/accessor/resolver behavior,
committed errors and updateAfterEvent eligibility. SDL uses this route after native
state and namespace callbacks. Observer-only keyboard publication prevents a
second implicit focused-clip call; the original focus-first API stays available. [EN](CLIP_INPUT_METHODS.md) /
[RU](CLIP_INPUT_METHODS.ru.md), [C89](../examples/embedding/clip_actions/README.md).

Русский: продвижение стадий 3/5/6/7/8/9; targeted hit transitions, lifecycle scheduler,
точный порядок оригинального Player и приёмка физических/чужих платформ остаются
открытыми. Все девять этапов открыты.

Validation for v0.627: 275 selected Release CTests and 93 whole-core ASan/UBSan
CTests passed on Linux (LeakSanitizer disabled in this environment). The updated
example compiled with strict C89/pedantic-errors and ran under ASan/UBSan, producing
Load=42, retained MouseMove=84, combined method=126 and coalesced redraw flags.
Ten new core regressions and two new actual-player CTests cover the new routes.
A separately linked fixture using the v0.626 retained-only SDL adapter rejected
the same SWF8 record/method trace movie as expected. Source links, deterministic
regeneration and debugger tutorial checks passed. SDL dummy drivers and synthetic
queue injection do not certify physical input, foreign OS or original-player order.

Проверки v0.627: 275 выбранных Release CTest и 93 whole-core ASan/UBSan CTest
успешны; пример strict C89 также выполнен под санитайзерами. Старый retained-only
SDL adapter ожидаемо отверг тот же movie с проверкой методов. Физические устройства,
чужие ОС и полный порядок оригинального Player не приняты; все стадии открыты.

## v0.626 retained scene input / Ввод для дерева клипов

Selected scene broadcasts now prepare root-first, depth-sorted recipients and
revalidate captured ancestry/removal before each retained global input delivery.
Depth/node/child/recipient bounds, heap-sort work, code callbacks and worker stop
share the outer budget. Newborns wait; removed/moved branches skip. The reentry/
teardown flag is context-local and restores across errors, exceptions and OOM.

The player uses the same SDL FIFO and native state/namespace publication before
scene MouseMove/Down/Up and KeyDown/Up records. Primary touch keeps its existing
mapping. A separate actual-SDL player fixture injects five queued transitions after
frame one and verifies their script sum plus one coalesced interframe redraw in
SWF5–8 interpreter/native JIT, with wrong-value controls.
[EN](CLIP_INPUT.md) / [RU](CLIP_INPUT.ru.md),
[C89](../examples/embedding/clip_actions/README.md).

Это продвижение стадий 3/5/6/7/8/9, не Press/Roll/Drag hit targets, KeyPress mapping,
полный lifecycle scheduler или оригинальный Player event-order certificate.
Физический input и видимые устройства/чужие ОС не приняты. Все девять этапов открыты.

Validation for this revision: 263 selected Release CTests and 83 ASan/UBSan
CTests passed on Linux. The updated embedding example also compiled as strict
C89 and ran under ASan/UBSan. A separately linked player using the previous
rootless pump rejected the same input-checking movie, as expected. Documentation
source links, deterministic regeneration and debugger tutorial checks passed.
These runs use SDL dummy drivers; physical devices and other operating systems
remain unverified.

Проверено в этой версии: 263 выбранных Release CTest и 83 ASan/UBSan CTest
на Linux; пример строгого C89 также выполнен под санитайзерами. Контрольный
проигрыватель со старым вызовом без дерева ожидаемо отверг тот же фильм.
SDL dummy не подтверждает работу физических устройств или других ОС.

## v0.625 retained ClipActions / Сохраняемые события sprite

PlaceObject2 sprite handlers now decode under strict SWF5–8 masks and bounded
record/action lengths. Managed code and selector graphs survive scene replacement,
duplication and transport. EnterFrame runs at the existing tick boundary; the C89
selected-event API delivers other records explicitly, without native onX/input or
automatic lifecycle scheduling. Prepared selectors/code survive callback mutation.

Preparation/action bytes, delivery and AVM1 share work and cooperative OS-worker
stop. Malformed metadata/hex and OOM unwind native/root/eligibility scopes; later
handler errors preserve completed effects. [EN](CLIP_ACTIONS.md) /
[RU](CLIP_ACTIONS.ru.md), [C89](../examples/embedding/clip_actions/README.md).

Validation: 251 selected Release CTest checks passed (232 core/example checks,
18 frontend checks and version refresh), plus 74 ASan/UBSan runtime checks with
LeakSanitizer disabled. The new tutorial also compiled as strict C89 and ran against
the instrumented core. Old v0.624 loader linkage rejects the positive wire fixture.
Source-link/determinism, tutorial comments, relative links and sorted glossary checks
passed. Native SDL dummy-video results are not visible-device acceptance.

Это развитие стадий 3/5/6/7/8 и Linux player-проверок стадии 9. Автоматические
load/unload/input targets, полный порядок оригинального Player, clipDepth и приёмка
видимых устройств/чужих ОС не добавлены. Все девять этапов остаются открытыми.

## v0.624 shared MovieClip ticks / Общий бюджет ticks

MovieClip advancement now shares one outer work scope across ticks, child staging,
initializers, frame segments and nested callbacks. Exhaustion/worker stop preserves
committed effects and skips the rest. Optional onEnterFrame delivery retains method
owner metadata through the existing event dispatcher; native/script handlers were
already callable. The native GC barrier remains active throughout traversal.

New tests cover owner/getter routes, lifecycle guards, exact/nested budgets, actual
OS worker stops, allocation failures, child mutation, transferred AVM1 handlers and
interpreter/native JIT scripts. Old v0.623 rejects owner-metadata and budget controls;
the superclass-call fixture passes old/new and is regression evidence only.
[EN](MOVIECLIP_TICKS.md) / [RU](MOVIECLIP_TICKS.ru.md),
[C89](../examples/embedding/movieclip/README.md).

Validation: 236 selected Release CTest checks passed (219 core/example checks,
16 frontend checks and version refresh), plus 62 ASan/UBSan runtime checks with
LeakSanitizer disabled. The new example also compiled as strict C89 and ran against
the instrumented core. Documentation source-link, relative-file, version metadata
and debugger tutorial checks passed.

Это ограниченная проверка стадий 3/5/6/7/8. Native helpers не становятся real-time
preemptible; полная приёмка ClipActions, оригинального Player и чужих ОС/устройств
не заявляется. Все девять стадий открыты.

## v0.623 updateAfterEvent and redraw / Внеочередная отрисовка

Standard installation now exposes a SWF5+ updateAfterEvent callback with stable
graph identity. Eligible interval, native Mouse move/down/up and focused MovieClip
keyboard call chains coalesce a host redraw flag; ordinary calls, standalone
Key/wheel/timeout routes do not grant eligibility. OOM/error/exception exits restore
the enclosing scope. Context-local flags remain stable through GC and do not export
with Function graphs. Three allocation-free C89 stage APIs expose host request/read/take.

Desktop redraw presents the current scene between frames without timeline ticks.
Separate actual SDL dummy-video tests and an actual-raster protocol capture fixture
verify requests/no-request controls in interpreter/JIT. The deterministic fixture
requires exactly 49 white redraws between red frames at 0/500 ms, despite two requests
per interval callback. A temporary v0.622 player loop misses every white redraw.

Validation: 88 selected Release CTest checks passed (71 core/example checks,
16 frontend checks and version refresh), plus 54 ASan/UBSan runtime checks with
LeakSanitizer disabled. The strict-C89 tutorial ran both immediate and real-clock
interactive paths against the instrumented core. Documentation source-link and
debugger example checks passed.

Контракты, ошибки, GC/графы и ограничения: [EN](UPDATE_AFTER_EVENT.md) /
[RU](UPDATE_AFTER_EVENT.ru.md), [C89](../examples/embedding/timers/README.md).
Это не полная ClipActions/target fanout или приёмка оригинального Player,
видимых устройств и Windows/macOS/Android. Все девять стадий остаются открытыми.

## v0.622 timer wakeup hints / Подсказки пробуждения

Added allocation-free [fvm_timers_next_delay](../include/flashvm/timers.h#L29) with atomic outputs, upward-rounded
capped waits, empty-queue clock independence, shared budgets and worker stop.
The desktop active loop now delivers timers and input between frames, caps SDL
waiting at a pending deadline and permits safe GC after interframe callbacks.
Timeline/render cadence and background suspension retain their existing policy.

Проверяются реальные SWF6–8 в SDL dummy-video player при 2 fps: интервал 10 мс
должен сработать несколько раз до второго кадра. Положительный фильм проходит
в interpreter/native JIT и отвергается временным player с прежним циклом v0.621.
Это подтверждение исправления цикла Linux, а не видимого окна, latency benchmark
или приёмки другой ОС/устройства. Контракты: [EN](TIMER_WAKEUP.md) /
[RU](TIMER_WAKEUP.ru.md), [C89](../examples/embedding/timers/README.md).

Validation: 78 selected Release CTest checks passed (65 core/example checks,
12 frontend checks and version refresh), plus 48 ASan/UBSan runtime checks with
LeakSanitizer disabled. The updated strict-C89 tutorial also ran both immediate
and real-clock interactive paths against the instrumented core. Source-link
freshness/determinism, tutorial comments/links and debugger checks passed.
All nine stages remain open; dummy video is not visible-device acceptance.

## v0.621 scheduled callbacks / Таймеры

Added setInterval/clearInterval for host/SWF6–8 and setTimeout/clearTimeout for
host/SWF8, with explicit owning-thread [fvm_timers_poll](../include/flashvm/timers.h#L19) delivery. Strong pending
and active value roots retain Function/capture, method owner and copied arguments;
self/other cancellation, new registration and GC during callbacks are supported.
Late intervals coalesce; timeouts consume before invocation. All four globals
have stable graph IDs and collecting-coercion restoration in imported Functions.

Очередь ограничена; регистрация, отмена, снимки и callback делят бюджет и
кооперативную отмену worker. Общий desktop player опрашивает её перед render;
изменённый C89-модуль компилируется с реальными заголовками SDL2 на Linux.
Это не приёмка видимого проигрывания, Windows/macOS/Android или точного расписания
оригинального Player. Полные ограничения: [EN](TIMERS.md) / [RU](TIMERS.ru.md),
[пример C89](../examples/embedding/timers/README.md).

Validation: 63 selected Release checks (61 runtime tests and two C89 examples)
and 46 ASan/UBSan runtime checks passed, with LeakSanitizer disabled. The new
C89 tutorial also passed both immediate and real-clock delayed interactive paths
linked against the instrumented core. The changed common desktop player module
compiled as C89 with real SDL2 headers. Documentation source-link freshness,
HTML/determinism and debugger tutorial checks passed; these are Linux results.

## v0.620 getTimer global / Глобальная getTimer

Standard installation now exposes the existing context clock as getTimer for
host/SWF4–8 execution. Global calls, extracted Functions, graph imports and
GetTime share the execution service without argument coercion. ID 163 preserves
older callback identities. See [EN](GET_TIMER.md) / [RU](GET_TIMER.ru.md) and the
[C89 tutorial](../examples/embedding/get_timer/README.md).

Глобальная getTimer использует существующие часы контекста; перенос Function
читает время получателя. Отдельно проверяются ошибки часов, GC-фазы, конфликты
имён, OOM и эквивалентность GetTime/глобального вызова/alias в JIT. Начало отсчёта
контекста и unsigned wrap не объявлены точной семантикой оригинального Player.
Таймеры-планировщики и приёмка остальных платформ остаются открытыми.

Validation: 53 selected Release checks (52 runtime regressions and the C89
example) and 37 ASan/UBSan runtime checks passed. LeakSanitizer was disabled.
Source-link freshness/determinism and debugger tutorial checks passed. These are
Linux results, not Windows/macOS/Android or original-player acceptance.

## v0.619 URL globals and trusted C calls / URL-функции и глобальные вызовы

Standard installation now publishes escape/unescape for host and SWF5–8 execution.
The bounded profile handles alphanumeric-only percent encoding, one-pass hex/plus
decoding, counted UTF-8/WTF-8, NUL and lone UTF-16 surrogates. Malformed percent
recovery and invalid decoded sequences remain explicitly unsupported; codepages
and original-player edge conformance are open. [EN](URL_FUNCTIONS.md) /
[RU](URL_FUNCTIONS.ru.md), [C89 tutorial](../examples/embedding/url_encoding/README.md).

Direct counted global C calls now honor trusted runtime collection policy and
root their copied capture through self-replacement, while preserving enclosing
native barriers. Standard-install preflight also checks own native methods and
uses active SWF case rules to preserve conflicting bindings. New URL graph IDs
161/162 do not alter old numeric IDs 150–156 or constant IDs 158–160; extraction,
clone-after-sender-destruction and collecting coercion are covered.

Linux x86-64 Release AGG/static dependencies: 49 selected checks pass (47 runtime
regressions, the real strict-C89 tutorial and version refresh). These include
standard object baselines, numeric/global/parser/GC/graph regressions and five
new fixtures: URL spellings/lifecycle/version/conflicts/work, actual SWF5–8
interpreter/manual/automatic JIT, old callback-ID identity and transported Function,
allocation sweeps with aliased output, and six native self-replacement/barrier/error
cases. The installer fixture includes 32 own-global/native-method conflict cases.

Minimal Debug core compiled throughout with ASan/UBSan: all 32 selected runtime
regressions pass with LeakSanitizer explicitly disabled. The same annotated C89
tutorial also runs against the instrumented VM, preserving eight UTF-16 units
including NUL. Cached executable materialization/execute permissions were restored
before the final runs; no not-started or runtime/assertion failure remains.
Source-link freshness, deterministic source-link/HTML and debugger tutorial
checks pass after documentation refresh. No fresh whole-core suite, reference
player, foreign native OS/device, full legacy malformed recovery, network/endurance
or completion of any roadmap stage is claimed.

Русский: это развитие стадий 1/2/5/6/7/8 — новые стандартные функции, согласованная
версионная семантика, корни GC, стабильные графовые ID и проверка JIT. Все девять
стадий, включая пункт 9, остаются открытыми.

## v0.618 input event budgets / Общий бюджет событий ввода

Key, Mouse and Selection now share one work scope across registration, detached
inspection, recipient capture, publication and handlers. Native/AVM1/JIT nested
calls cannot reset the enclosing budget. Add/remove precharge relocation or
compaction; failed registration preserves roots/order and native remove output.
Before-publication failure preserves valid state; later failure preserves new
input/focus and completed effects. Polling honors real worker stop requests even
when the numeric limit is disabled. [EN](INPUT_EVENT_BUDGET.md) /
[RU](INPUT_EVENT_BUDGET.ru.md), [C89 tutorial](../examples/embedding/event_work_budget/README.md).

Linux x86-64 Release AGG with static dependencies: all 25 selected event/listener,
allocation, focus, wheel/queue and JIT regressions pass. Four new tests exercise
135 version/namespace/route/failure combinations, publication boundaries with GC,
36 version/mode/event cases with real script handlers and checked native entries,
and ten actual OS workers stopped before or between callbacks. Numeric-limit
exhaustion and worker cancellation both restore delivery/GC/execution scopes.
The strict-C89 tutorial's actual CMake target runs with three listeners and a
five-unit budget, prints callbacks=1 pointer=23,45, and cleans up correctly.
The independent build-version-refresh check passes as well.

A minimal Debug VM compiled throughout with ASan/UBSan passes the same 25
regressions; LeakSanitizer is explicitly disabled. The tutorial also runs against the instrumented VM. Source-link freshness,
deterministic HTML/link checks, annotated tutorial/catalog/link validation and
debugger tutorial checks pass. This is selected
regression evidence, not a fresh full-core suite or native foreign-platform,
physical-device, original-player, cancellation-race/endurance certification.
All nine stages remain open.

Русский: это продвижение стадий 3/5/6/7/8 — общий бюджет исполнения событий,
сохранность состояния при ошибке, реальные worker-потоки, GC и JIT. Оно не
закрывает пункт 9 или полную совместимость SWF8.

## v0.617 registration work budget / Бюджет регистрации слушателей

AsBroadcaster add/remove charge logical identity scans, copied stored indices and
new insertion to the same execution budget as nested AVM1 and broadcast work.
Interrupted staging preserves the live Array's identity, length and entries.
Duplicate/missing searches cannot bypass the limit; outer scopes are not reset.
[EN contract](AS_BROADCASTER.md#registration-work-limits-v0617) /
[RU](AS_BROADCASTER.ru.md#ограничение-работы-регистрации-v0617).

Fresh Linux x86-64 Release C89/C++98 AGG build with bundled static dependencies:
eight selected CTest checks pass (six broadcaster fixtures, independent-process
transport and build-version refresh). The new fixture covers 45 combinations of
SWF6–8, direct/extracted native calls, interpreter/manual/automatic JIT and failed
search/copy/compaction; each retries after GC and checks nested shared budgets.
Additional direct checks exercise exact insertion/removal boundaries, missing
removal and unlimited numeric budgets. Native entries are checked by the shared
JIT fixture; these are actual x86-64 runs, not foreign-ISA acceptance.

A fresh minimal Debug VM built throughout with ASan/UBSan passes all seven
broadcaster/independent-process checks, with LeakSanitizer explicitly disabled.
The unchanged annotated tutorial was compiled as strict C89, linked to the
Release library and run: onValue 42 locally and 84 in its destination context.
Source-link freshness, deterministic source-link/HTML tests and debugger tutorial
checks pass. No complete core suite, original-player comparison, physical device,
worker cancellation race or new Windows/macOS/Android run is claimed.

Русский: проверены границы общего бюджета и сохранность опубликованного списка
при прерывании поиска и подготовки карты. Это продвижение стадий 5/6/7/8, не
полная приёмка платформ или всей семантики событий. Все девять стадий открыты.

## v0.616 broadcaster transports and player / Каналы событий и плеер

Broadcaster graph acceptance now includes bounded memory FIFO, durable files and
loopback TCP queues across SWF6–8 in interpreter/manual/automatic JIT modes. A
script listener calls gc.collect after the sender was reset and destroyed; cyclic
edges, shared cached Function identity, payload/counters and subscription reset
are checked. Separate file writer/reader processes test independent address spaces.
The annotated C89 tutorial copies a native-listener broadcaster into a second
context with explicit callback registration and destination GC.

The shipped common player installs the opt-in AsBroadcaster namespace for SWF6–8.
Self-checking CLI movies cover those versions and SWF5 startup in interpreter/JIT
modes, plus a wrong-payload negative control. SDL dummy video/muted audio evidence
is software integration, not native device or foreign-OS acceptance. Core objects
remain independent of SDL/Clutter3d/GTK/Qt; no Clutter3d library changes are needed.
[EN](AS_BROADCASTER.md#transport-and-player-v0616) /
[RU](AS_BROADCASTER.ru.md#перенос-и-плеер-v0616).

Validation: 22 targeted CTest cases pass on Linux x64, including broadcaster
transport/process cases, prior graph/channel codecs, independent standard-object
file/network processes, player input/options and muted sound-bearing movies,
new player interpreter/JIT fixtures and version/tutorial checks. Strict C89 builds
and the updated two-output tutorial pass. ASan/UBSan transport/process and player
fixtures pass with the broadcaster bridge and player/fixture code instrumented,
ordinary VM/SDL libraries and leak detection disabled. Documentation source links
and individual API/comment coverage pass; no full-suite or hardware claim.

Worker races, remote network failures/endurance, historical Flash semantics and
full platform acceptance remain open. All nine roadmap stages remain open.

Русский: продолжена приёмка этапов 5–8 через реальные каналы и отдельные процессы;
общий плеер получил установку namespace, а пример — независимую копию графа.
Программные smoke-проверки не означают приёмку Android/Windows/macOS или устройств.

## v0.615 generic application broadcasters / Пользовательские события

Explicit AsBroadcaster namespace and same-context initialization add a read-only
live Array and add/remove/broadcast methods. Rooted snapshot dispatch preserves
counted event names and actual handler errors. Allocation sweeps cover publication,
reset, registration changes and pre-delivery preparation. Global Object/Array
validation now preserves lookup failures rather than replacing OOM with invalid
argument; initialization roots its receiver before that potentially scripted lookup.
Stable callback identities retain cyclic graphs and cached method wrappers after
sender destruction. Loaded SWF6–8 interpreter/manual/automatic JIT fixtures include
nested script GC and actual native-entry checks on the Linux x64 JIT host.

[Contract EN](AS_BROADCASTER.md) / [RU](AS_BROADCASTER.ru.md),
[annotated C89/AS tutorial](../examples/embedding/broadcaster/README.md).
Validation: 275 selected VM tests pass, including the four new broadcaster
fixtures; nine restored ELF fixtures needed execute permissions and then passed
on retry. The isolated build-version check and documentation source-link checks
pass. The tutorial compiles as strict C89 and prints onValue: 42. Broadcaster
bridge ASan/UBSan passes host, graph and JIT fixtures with leak detection disabled;
this instruments the new bridge, not the whole VM. No full-suite/platform claim.

Historical list mutation/duplicate/primitive/reflection rules and complete reference,
channel/worker and foreign-platform acceptance remain open. This is progress in
stages 1/2/5/6/7/8; all nine stages remain open.

Русский: добавлен независимый от frontend объект событий, атомарная подготовка,
проверки ошибок памяти, GC, циклического графа и настоящих AVM1/JIT-вызовов.
Полная совместимость с оригинальным Flash и приёмка всех платформ не заявляются.

## v0.614 coherent evdev primary state / Общее удержание evdev

BTN_LEFT and BTN_TOUCH now retain independent copied native state. Mouse down/up
is delivered only when their combined OR changes, preventing duplicate presses
and premature release while the other source remains held. Recovery snapshots
populate the same bits; autorepeat and unrelated keys/buttons remain ignored.

The C89 evdev_buttons fixture checks real public Mouse listener counts for both
source orders, duplicates, autorepeat and unrelated records; evdev_recovery adds
release after a restored touch. Linux adapter ASan/UBSan uses disabled leak
detection and the ordinary VM library, not whole-VM instrumentation. This is a
single-primary host policy, not full multitouch or physical-device acceptance.
All nine stages remain open.

Русский: исправлено объединение двух нативных источников в один указатель VM;
контракты ядра/ABI не изменены. Аппаратная и межплатформенная приёмка остаётся
открытой. [Details / Подробности](../examples/frontends/linux_fb/README.md#v0614-primary-source-reconciliation--согласование-кнопок).

## v0.613 Clutter wheel and stage cursor / Колесо и курсор Clutter

The requested fork's public Clutter/Cogl API now drives vertical discrete/smooth
wheel through inverse actor projection and the existing VM line-delta boundary.
Touch-only/active-touch, horizontal and zero-event policies remain explicit.
Cursor requests are applied to the owned stage before upload, with a cached
stage-local show/hide request and --hide-cursor startup option. The library and
excluded inferno-os client are unchanged; no additional library patch is required.

A real Linux Clutter 1.26 auxiliary SDK fixture verifies deltas, coordinates,
contact ownership and cursor properties, plus three 120-frame runs (ordinary,
hidden cursor, touch-only). The full existing fractional projection fixture on
the unpatched system library still reproduces the already documented precision
bug; it is retained as a strict patched-fork regression, not weakened or claimed
passing. Public declarations were checked in the user's fork. Fork-specific
Android/GLES/device and hardware cursor acceptance remain open. All stages remain
open; no auxiliary SDK result substitutes for native platform acceptance.

Русский: новые привязки проверены на настоящем Linux SDK; это не свежая сборка
пользовательского Android-форка и не проверка телефона. Строгая проверка дробной
3D-проекции сохранена; системная библиотека без прежнего патча её не проходит.
[API, parameters and ownership](../examples/frontends/clutter3d/README.md#v0613-wheel-and-cursor--колесо-и-курсор).

## v0.612 evdev snapshot recovery / Восстановление evdev

The direct Linux tutorial now cancels an old primary contact on SYN_DROPPED,
discards pending wheel/unpublished relative motion, ignores damaged input through
the next SYN_REPORT, then queries current key state and both calibrated absolute
axes where applicable. A held snapshot starts a new contact; a released snapshot
publishes the recovered position. Lost relative/wheel displacement is not guessed.
Native ioctl retries EINTR; failed queries and invalid calibration fail closed.
Sequential snapshots are not an atomic hardware transaction.

Two current Linux C89 decoder tests pass: linux_device_layout and evdev_recovery.
The recovery fixture also passes adapter ASan/UBSan (LeakSanitizer disabled in the
traced environment); that run links the ordinary VM library and does not claim
a newly instrumented whole VM. Fixtures cover ignored stale/repeated-loss records,
relative position preservation, held absolute recovery, per-query disconnects,
invalid ranges and coordinate clamping. Physical fbdev/evdev devices and other OS
acceptance remain open. All nine roadmap stages remain open.

Русский: это восстановление существующего Linux-адаптера после переполнения
очереди, без повторения потерянных нажатий/прокрутки. Проверен реальный декодер
с контролируемыми ioctl-снимками, не физическое устройство.
[Contract / Контракт](../examples/frontends/linux_fb/README.md#v0612-evdev-loss-recovery--восстановление-evdev).

## v0.610 native cursor application / Курсор оконных хостов

SDL2, GTK3 and Qt5 tutorials now poll the context request before paint and apply
changed visibility through their actual native SDK. --hide-cursor sets a startup
request and combines with --smoke. SDL saves/restores its global visibility under
a standalone one-window policy. GTK owns a lazy same-display blank descriptor,
restores inheritance, and releases its reference after native window teardown.
Qt uses widget-local blank/inherited cursor. Frame marker/content is unchanged.

Three current real Linux SDK adapter tests pass (SDL dummy-video, GTK3/Xvfb,
Qt5 offscreen), including enable/hide/repeat/show state and native descriptor/
property queries. Each host completes both ordinary and hidden 120-frame smoke
runs (six total runs). Source-link determinism, C89/C++98 core / SDK-specific
compilation, tutorial/API checks and version-refresh checks pass. Native SDK
setters returning void prove issuance/property change, not hardware/compositor
acknowledgement. No full-suite or foreign-platform retest is inferred.

Русский: видимость применяется к системному курсору отдельно от собственного
цветного маркера. Проверены настоящие SDK в программной среде; физическая мышь,
тачскрин, Clutter/direct-fb cursor adapters, другие ОС, многооконное владение
SDL-курсором и полная приёмка стадий остаются открытыми.
[Frontend API/lifetime](../examples/frontends/README.md).

## v0.609 Mouse.hide/show / Скриптовая видимость курсора

Explicit Mouse installation now includes hide/show. Both ignore arguments without
coercion, change the context request and return previous 0/1 as a Number. Cursor
methods support SWF5–8; listener callbacks retain their SWF6 guard. Listener members
remain present but unsupported in SWF5, an explicit reflection limitation.

Six targeted Linux Release tests pass: jit_mouse_cursor, jit_mouse,
mouse_allocation, mouse_cursor, mouse_listeners and the extended C89 tutorial.
The two JIT fixtures and allocation fixture also pass ASan/UBSan without leak
detection. Real SWF5–8 AVM1 calls execute in interpreter/manual/automatic JIT with
native-entry evidence, previous-state sequence 1/0/0/1, ignored coercion traps,
GC, shadow conflicts and preinstalled SWF4 rejection. Namespace installation OOM
remains atomic; direct cursor callbacks work under allocation denial.

Русский: скриптовый запрос проверен, физический курсор не заявляется. Точное
историческое наличие членов Mouse в SWF5 и нативные окна/ОС остаются отдельной
приёмкой. [Script contract](MOUSE.md#script-hide-and-show).

## v0.608 context cursor request / Контекстный запрос курсора

Allocation-free C89 get/set cursor visibility APIs now own a per-context requested
0/1, initially visible. Invalid lifecycle/arguments preserve state/output;
requests neither call an OS nor mutate input, focus or subscriptions. Five Linux
Release tests pass: mouse_cursor, mouse_allocation, mouse_listeners,
mouse_wheel_queue and the mouse_cursor C89 tutorial. The first two also pass
ASan/UBSan without leak detection. Tests cover context isolation, repeated sets,
movie-load persistence, callback/GC use and allocation denial.

Русский: это запрос VM, не подтверждение видимости на экране. Скриптовые hide/show
и оконное применение на этой точке ещё не заявляются; устройства и все стадии
остаются открытыми. [Contract](MOUSE.md#cursor-visibility-request).

## v0.607 native wheel adapters / Колесо во фронтендах

SDL2, GTK3, Qt5 and direct Linux evdev tutorials normalize their native vertical
wheel protocols into the explicit VM line-delta API. Common native diagnostics
record successful deliveries and last delta without changing held-button state.
The tutorial policy is three lines per detent; this does not query OS scroll
preferences. SDL preserves preciseY when available, handles FLIPPED and uses last
accepted logical VM position for baseline protocol compatibility. GTK uses event
position and signed smooth/discrete conversion. Qt converts angle units and leaves
zero/horizontal/pixel-only input unhandled. evdev queries high-resolution axis
capabilities, suppresses legacy duplicates even across separate reports, and
commits wheel only at SYN_REPORT. Three-byte mousedev and Clutter gain no native
wheel decoder in this block.

Four current Linux SDK/adapter tests pass: example_sdl_input (real SDK dummy
video), example_gtk3_input (real SDK/Xvfb), example_qt5_input (real SDK offscreen)
and linux_device_layout (synthetic device records/layout). Assertions cover
fractional scale/line conversion, SDL inversion/touch duplicate suppression, Qt
pixel-only rejection, evdev high-res/legacy selection and dropped packets.
GTK3, Qt5 and SDL tutorials each complete a real 120-frame software smoke run.
The Linux decoder/common wheel failure-latch fixture also passes ASan/UBSan
without leak detection. Strict C89 Linux syntax, source-link determinism, tutorial
checkers and version-refresh checks pass. No whole-suite or foreign-device run
is inferred from these targeted checks.

Русский: нативные единицы переводятся явно, дробь сохраняется. Четыре целевых
адаптерных проверки прошли, три оконных примера завершили по 120 кадров. Физические
колёса/тачпады, framebuffer/камера, Android, Windows/macOS, старые SDK и Clutter
этим блоком не приняты. Полный scrollTarget/сцена и релизная приёмка стадий
остаются открытыми. [Frontend guide](../examples/frontends/README.md).

## v0.606 queued wheel and native execution / Колесо в FIFO и JIT

Wheel is now a native copied variant in the shared 1024-entry FIFO, with event-
time world position and finite signed/fractional delta. Eight targeted Linux
Release tests pass: mouse_wheel_queue, jit_mouse, mouse_wheel, input_mixed_queue,
input_queue, mouse_allocation, mouse_listeners and the extended mouse_wheel C89
tutorial. Five tests also pass ASan/UBSan without leak detection: wheel queue,
wheel direct, mixed queue, jit_mouse and allocation. Allocation denial covers
empty-queue append and growth with old entries retained.

The real SWF6–8 fixture reads the wheel parameter under interpreter/manual JIT/
automatic JIT, checks actual native entry counters, collects inside handlers,
and validates direct/queued script exception recovery. Native queue tests cover
mixed Key/gesture/pointer/wheel ordering, entry limits, callback additions,
cancel/refill epochs, failed-event consumption, remaining work and saturation.

Русский: колесо встроено в общую очередь без объектов/указателей событий внутри
записи. Проверены последовательность, ошибки/OOM, GC и интерпретатор/JIT на
SWF6–8. Эталонный scrollTarget, устройства, новые оконные SDK и чужие ОС этим
блоком не приняты. [Queue contract](INPUT_QUEUE.md).

## v0.605 direct wheel delivery / Прямая доставка колеса

[fvm_input_dispatch_mouse_wheel](../include/flashvm/mouse.h#L36) adds finite signed/fractional line deltas through
onMouseWheel(delta), reusing rooted Mouse snapshots, coordinate/drag publication
and cross-event recursion/lifecycle protection. Six targeted Linux Release tests
pass: mouse_wheel, mouse_listeners, mouse_allocation, jit_mouse (existing pointer
fixture), input_mixed_queue and the new C89 mouse_wheel tutorial. The first three
also pass ASan/UBSan with leak detection disabled. Wheel-specific script/JIT and
queue/frontend delivery are not claimed by this checkpoint.

Русский: проверены числовой аргумент, GC при изменении подписок, запрет вложенных
Mouse/колеса и разрушения контекста, ошибки/OOM до и после публикации, drag,
невызываемый обработчик и неверные значения. Автоматический scrollTarget,
исторический порядок/совместимость Windows и физические устройства остаются
открытыми. [EN](MOUSE.md#wheel-events) / [RU](MOUSE.ru.md#колесо-прокрутки).

## v0.604 GTK3, Qt5 and Linux devices / Новые фронтенды

Three opt-in executable tutorials now live in examples/frontends/gtk3, qt5 and
linux_fb. GTK3 consumes opaque RGBA through GdkPixbuf/Cairo and handles logical
resize/pointer/focus loss. Qt5 uses scoped QImage/QPainter and native QWidget
events/deactivation; only this SDK host uses C++11, while the VM remains C++98
and its public ABI C89. Direct Linux uses actual fbdev mmap, one explicit
evdev or mousedev stream, and optional single-plane progressive YUYV V4L2
MMAP capture with independent copied grayscale preview. No SDK becomes a core
dependency; NO_GRAPHICS and non-Linux device configurations fail explicitly.

Current Linux evidence: all three production hosts and all three adapter fixtures
build. The GTK3 input/paint fixture passes with a real Xvfb X11 display; the Qt5
fixture passes with its real offscreen SDK plugin and verifies actual painted
marker colors after native events. Both window examples complete 120-frame
smoke runs with status=0. The Linux device layout/decoder fixture passes in
Release and ASan/UBSan (leak detection disabled): bitfield/span checks, virtual
row offset/padding writes, YUYV pitch/truncation, partial signed mousedev packets,
evdev movement and SYN_DROPPED cancellation. Invalid Linux options and /dev/null
as framebuffer are rejected. A renamed detached component configures without
window SDKs; requesting GTK under NO_GRAPHICS fails before SDK discovery.
Build-version refresh, source-link determinism and tutorial checkers pass.

Русский: три новых исполняемых примера собраны и документированы на двух языках,
каждая строка исходника пояснена, API перечислены с параметрами и ссылками.
Проверены настоящий GTK3/Xvfb и Qt5 offscreen, по 120 кадров в оконных примерах;
Linux-проверка использует тестовые структуры/буферы, а не физические устройства.
Реальные /dev/fb0, evdev/mousedev, камера, аппаратный экран/тачскрин, Android,
Windows/macOS и статические релизные SDK этим блоком не приняты. Камера даёт
только копию сырой яркости Y, не цветное преобразование. Все стадии остаются
открытыми; готовность полной Flash/SWF системы не заявляется.

See [frontend catalog and API](../examples/frontends/README.md).

## v0.603 mixed input FIFO / Общая очередь ввода

Gestures, Mouse and copied Key snapshots now share the context FIFO, with
world coordinates captured at enqueue time. Pumping handles only its entry
batch; callback additions wait for the next pump. Explicit cancellation
invalidates the old batch while preserving events appended after cancellation.
Recursive pumping and context destruction during dispatch are blocked.

Eight targeted tests pass in Linux Release and a separate ASan/UBSan build
(leak detection disabled): input_callback_failures, input_mixed_queue,
input_pointer, input_queue, input_validation, jit_mouse, and the input_queue
and mixed_input embedding tutorials. The JIT fixture covers real SWF6–8
interpreter/manual/automatic native delivery, queued GC and exception recovery.
The build-version refresh and tutorial/source-link checks also pass.

Жесты, Mouse и копии состояния Key используют общую очередь контекста.
Проверены порядок, координаты, отмена, добавление из обработчика, лимит,
ошибки, GC и восстановление после исключения. Восемь целевых тестов прошли
в Linux Release и отдельно с ASan/UBSan без проверки утечек. Новых запусков
SDL/Clutter SDK, Android или Windows в этой версии нет. Перевод всех
фронтендов на очередь и полная приёмка стадий остаются открытыми.

See [INPUT_QUEUE.md](INPUT_QUEUE.md) and the
[mixed input tutorial](../examples/embedding/mixed_input/README.md).

## v0.602 standalone Clutter patch / Отдельный патч библиотеки

The standalone git-format patch is copied into the Clutter frontend example,
with repository-specific application instructions and scope documented in
[LIBRARY_PATCH.md](../examples/frontends/clutter3d/LIBRARY_PATCH.md).
It changes only library configure/actor sources, excluding inferno-os.
Это отдельная доставка уже описанных исправлений библиотеки; она не означает
новую проверку SDK или готовность Android/Windows.

## v0.601 GLES2 and projected input / GLES2 и проекция ввода

The same selected fork now builds a separate static Cogl/Clutter Linux SDK
with Xlib-EGL/GLES2 enabled and desktop GL/GLX disabled. Its source/prefix
renderer guard rejects mixed Autotools trees, and CMake requires both Cogl
packages to come from the same explicit SDK as Clutter.

The real 3D fixture exposed integer truncation in the fork's actor inverse.
The new upstream-source patch preserves fractional event coordinates and
allocation dimensions without changing ABI. Both SDK profiles rebuilt with
that fix; CMake verifies its hash. Linux Xvfb/Mesa GLX and EGL/GLES2 fixtures
passed RGBA texture readback, first-touch/cancellation and inversion of a
25-degree Y-rotated actor with fractional allocation. GLES2 touch-only smoke
completed 120 uploads. The GLX fixture passed again in the ASan/UBSan host/VM
build (SDK uninstrumented, leak detection disabled); SDL/headless and two
Python checks also passed in that build. Guard rejection and documentation
checks passed. These are bounded Linux/software results, not Android, hardware
GLES or complete original-player/platform acceptance. All stages remain open.

Проверены именно исходники выбранной библиотеки на GLX и EGL/GLES2, включая
исправленную дробную обратную проекцию 3D-актёра. SDK не содержит inferno-os.
Телефон, ANativeWindow/Activity и аппаратный GLES пока не проверены.
[SDK details](../external/clutter3d/README.md), [tutorial](../examples/frontends/clutter3d/README.md).

## v0.600 frontend tutorials / Примеры фронтендов

SDL and the selected bhgv Clutter/Cogl fork now have annotated C89 tutorials
under examples/frontends; the old headless host moved there too. The independent
fork SDK export excludes inferno-os and preserves upstream licensing. Two
configure repairs unblock modern Linux static builds: unconditional C++ libtool
initialization and omission of the unused missing Win32 template.

A clean export rebuilt both fork Cogl/Clutter 1.14.1 as static libraries. The
example and actual SDK actor/input/texture-readback fixture passed under Linux
Xvfb/Mesa, plus two independent 120-frame runs (normal and touch-only). SDL
software pixels/first-contact/reset tests and the 120-frame dummy-video run
passed. Four targeted CTest checks in the ASan/UBSan build plus the separate Clutter SDK
fixture passed (leak detection disabled; third-party SDK remains uninstrumented).
Three checks passed from a renamed detached VM component (SDL, headless,
tutorial contracts). Four default-core checks passed (headless, version refresh,
source links, tutorial contracts); NO_GRAPHICS explicitly skips the headless
raster run. No full-suite rerun, physical GPU, Android or foreign-OS acceptance
is claimed. [Tutorials](../examples/frontends/README.md) / [SDK](../external/clutter3d/README.md).

Проверены реальные библиотеки выбранного Git на Linux/Xvfb/Mesa, а не только
системный Clutter. Клиент исключён. Android/GLES surface lifecycle и приёмка
на телефоне остаются открытыми, как и стадии 1–9.

## v0.599 runnable host / Исполняемый хост

The no-SDL C89 tutorial renders and consumes a real frame, maps host coordinates,
and delivers down/up into the VM. SDL's mixed FIFO now uses that same inverse
mapping. Three targeted default Release checks pass (host, tutorial, build-version
refresh), seven lean SDL Release checks pass, and seven targeted ASan/UBSan
checks pass with leak detection disabled. A separately configured NO_GRAPHICS
build, without SDL, passes the host contract test and explicitly skips the
raster tutorial (77). Documentation links, debugger tutorials and the new
example's comments/catalog ordering pass. No full-suite or foreign-device
rerun is claimed. [Tutorial](../examples/frontends/headless/README.md).

Пример и общее преобразование ввода проверены в Release и ASan/UBSan;
отдельная сборка NO_GRAPHICS без SDL проверяет host API и явно пропускает
растровый пример. Полный набор и чужие платформы заново не заявляются.

## v0.598 independent host / Независимый хост

The C89 frame consumer ABI and inverse affine event mapping are independent of
SDL/Clutter/GLES; the SDL player now uses the same bounded frame submission.
Seven targeted Linux Release tests pass: host geometry/buffer/error contract,
SDL pointer and keyboard delivery, actual software texture pixels under dummy
video, and three shipped sound-bearing movies in each execution mode with mute.
Six targeted ASan/UBSan checks also pass (host, SDL pointer/movie keyboard,
software pixels and muted interpreter/JIT), with leak detection disabled.
This is software/backend acceptance, not a native window or Android/GPU run.
[EN contract](HOST_BACKENDS.md) / [RU](HOST_BACKENDS.ru.md). The referenced
Clutter shell was inspected outside inferno-os; its actual adapter remains future
work. All stages remain open. Historical full-suite counts are not a new run.

Проверены семь целевых тестов Linux Release, включая реальный SDL с dummy-video
и оба режима исполнения роликов. Это не запуск на Android/GPU; адаптер Clutter
ещё не реализован. Полный прежний набор тестов заново не заявляется.

## v0.583 verification / Проверка v0.583

Default Linux Release: 1235 passed, four explicit platform/service skips out of
1239 registered. Eight separate real-SDL frontend tests passed under X11/Xvfb,
including input/pixels, both execution modes, prepared StopSounds, a 350 ms
render-thread stall and simulated 8192-frame obtained-quantum policy. The latter
uses real SDL with wrapped geometry, not an Android device. Four new stop-binding
core tests also passed ASan/UBSan with leak detection disabled. The no-device
C89 tutorial reports `pending=0 stops=1`. SDL SDK queue geometry checks passed.

Android ARM64 v0.583: real NDK/SDL native build and test APK with signature and
16 KiB ELF/ZIP alignment checked. The user reported sporadic knocks/crackling
in v0.582 on Android. The worker/reserve change addresses an identified underrun
scenario; actual listening confirmation on that phone remains outstanding.
No Windows/macOS runtime or stage-9 completion is inferred.

## Current Linux verification / Текущая проверка Linux

v0.582: complete default Linux Release C89/C++98 build, including examples;
1230 passing tests and 4 explicit skips out of 1234 registered. Skips require
native macOS ARM64/Intel, native Windows unwind, or an external speech SSIP endpoint.
The separate lean AGG/JIT SDL2 player passes five explicit frontend checks under
X11/Xvfb, including real keyboard/mouse injection and pixel changes in both modes.
Both delivered SWFs complete 120 frames in interpreter and JIT; interactive JIT
reports native=120/interpreter=0/fallbacks=0. SDL disk output in both modes contains
PCM8/PCM16/ADPCM and expected mono/stereo tone frequencies. Dummy/disk audio checks
are not physical listening acceptance. Core default tests have desktop SDL OFF.

Windows x64: real MinGW GCC13 C89/C++98 PE32+ player build and DLL-import inspection;
no actual Windows runtime run. Android arm64-v8a/API23: NDK r27c + SDL2 2.30.0,
SDLActivity Java/Dex and test-signed APK, 16 KiB ELF/ZIP alignment and signature
verified; no actual Android device/emulator execution. SDK sensor support disabled
for the older SDL2 source's NDK27-incompatible ALooper_pollAll call. This does not
replace native platform/device/endurance gates.

Полная core-сборка Linux: 1230 успешных, 4 пропуска из 1234. Отдельный player:
5/5 X11/Xvfb проверок, реальные input/pixel изменения и interpreter/JIT обоих SWF.
PCM записан настоящим SDL disk backend и проверен по частотам в обоих режимах.
Windows/Android собраны, но на целевых ОС не запускались. Подпись/ABI/выравнивание
APK и кросс-компиляция не доказывают нативную совместимость.

## Current work and next boundary / Текущая работа и следующий рубеж

v0.582 connects the desktop host to a documented HostInput extension and root
PCM/ADPCM event queues; --mute avoids device opening. Adds reproducible lean
player profile/cross-toolchain, Android SDLActivity test menu and two standalone
SWF fixtures. [EN](PLAYER_TEST_BUILDS.md) / [RU](PLAYER_TEST_BUILDS.ru.md).
Wall-clock video and PCM FIFO clocks remain independent. Next: full Flash
Key/Mouse/button/event routing, StopSounds/AS Sound integration into this player
mixer, drift/latency/underrun policy, sprite sound/linkage and stream codecs.
Native Windows/Android/macOS device runs and complete reference acceptance remain
open. All nine stages remain open.

v0.582 даёт настоящие тестовые player binaries/APK, input snapshot и root event
звук. Полный стандартный input/audio API, A/V синхронизация и приёмка целевых
платформ ещё открыты; номер версии не означает завершение стадий.

## v0.577 desktop checkpoint / Десктопный запуск

The optional [SDL2 desktop host](DESKTOP_FRONTEND.md) adds OS launch adapters,
JIT/interpreter selection, quality, monotonic frame pacing and playlists.
Stage bounds and a render-only viewport preserve authored transforms.
Linux X11/software presentation is exercised with real windows, including
RGBA pixels, close events and FWS/CWS playback. Cairo core viewport/shape/RECT/
raster tests also pass (4/4); this build does not require SDL.
The eight documentation-viewer tests pass. Foreign-OS launch adapters are
implemented but not native-tested. All nine stages remain open.

Добавлен [десктопный host](DESKTOP_FRONTEND.ru.md). Проверены окно X11,
пиксели, закрытие, FWS/CWS, JIT/interpreter и управление темпом.
Cairo: 4/4; просмотрщик документации: 8/8. Windows/macOS требуют
нативного прогона, Android — Activity/surface frontend. Это не закрывает
полноту SWF, input, звуковой clock и платформенную приёмку стадии 9.

Historical v0.577 desktop-enabled Linux profile: 1140 passed and 3 explicitly skipped
(native macOS ARM64/Intel and Windows unwind), 1143 registered tests total.
This run disables examples and is not directly comparable to v0.576's count.
Исторический Linux-профиль v0.577 с desktop: 1140 успешных, 3 пропущенных, всего 1143;
examples выключены, поэтому число тестов отличается от предыдущего профиля.

## v0.578 incremental event mixing / Порционное смешивание событий

Independent software polyphony now combines prepared PCM/ADPCM snapshots with
per-ID stop/no-multiple, loop/envelope progress, fixed geometry and bounded voice
capacity. Wide summation clips once; reads allocate nothing and survive source
reload/GC/context/runtime destruction. See [EN](SWF_SOUND_MIXER.md) /
[RU](SWF_SOUND_MIXER.ru.md) and the annotated C89 tutorial.

Five focused mixer/tutorial tests pass in the Linux minimal-JIT ASan/UBSan build
(LeakSanitizer disabled). This includes injected OOM cleanup, all ADPCM widths,
packet/chunk boundaries, exact native file PCM and strict C89 use. Source links,
debugger tutorial contracts and the new example's line comments/catalog/anchors
are checked separately. No native device or foreign-OS acceptance is claimed.

Это развитие стадий 3/4/7, а не закрытие любой из девяти стадий. Следующие
звуковые рубежи: фактическая доставка исполняемых timeline-событий в host mixer,
разные частоты/каналы, системный device clock, streaming codec/A/V integration.
Общее число тестов не заменяет эти условия и оригинальные Flash-эталоны.

## v0.579 root frame sound delivery / Звуки фактического входа в кадр

Root DefineSound/StartSound now import and deliver ordered event ordinals through
a synchronous host callback before DoAction. Visual seek reconstruction does not
replay skipped sound frames. Loaded source bytes are checked against the imported
graph, including after reload or transport; callback pointers remain host-local.
The C89 mixer tutorial adds --timeline-demo with real frame-driven submission.
See [EN](MOVIECLIP_SOUND.md) / [RU](MOVIECLIP_SOUND.ru.md).

Следующий рубеж: sprite sound, эффективный prepared source index, resampling и
подключение к реальному device clock. Callback сам не создаёт устройство и не
синхронизирует видео/звук. Все девять стадий остаются открыты.

Final v0.579 verification: full default Linux Release build/CTest, 1216 passed
and 4 explicit skips out of 1220. Focused timeline/OOM/transport/validation/tutorial
tests: 5/5 in ASan/UBSan, LeakSanitizer disabled. No SDL window, audible device
or foreign native rerun is claimed. Source links and example contracts pass.

## v0.580 output conversion / Преобразование выхода

A separate configured creation ABI enables rational-phase linear conversion to
5512..192000 Hz mono/stereo. The legacy strict entry point and info ABI are
unchanged. Different source geometries, loop duration, buffered EOF, slot reuse,
stop/no-multiple and final wide-sum saturation are covered. Eight focused
conversion/timeline/tutorial tests pass in Linux ASan/UBSan (LeakSanitizer off).
An actual strict-C89 client compiled against the v0.579 mixer header also links
and runs against the new library. [EN](SWF_SOUND_CONVERSION.md) /
[RU](SWF_SOUND_CONVERSION.ru.md). This is software conversion without a low-pass
anti-alias filter or device/foreign-platform acceptance.

Final v0.580 verification: full default Linux Release C89/C++98 build/CTest,
1223 passed and four explicit native-macOS/native-Windows/SSIP skips (1227 total).
Eight focused tests pass in minimal-JIT ASan/UBSan with LeakSanitizer disabled.
Legacy-header ABI client, source links, guide anchors, tutorial line comments/no
assertions, catalog entries and debugger tutorial contracts pass separately.
No native audio device, SDL window or foreign-platform rerun is claimed.

## v0.584 diagnostic checkpoint

Native frontend-only changes: optional bounded SDL-input capture and Android
private read-only ZIP export. Ten frontend checks passed: nine SDL/X11/Xvfb checks plus bounded storage/error
verification. Actual accepted WAV in both interpreter/JIT preserves expected stereo
tones and matching non-clipped peaks; StopSounds markers are recorded. The storage
fixture also passed strict C89 compilation and ASan/UBSan (leaks disabled).
Windows x64 and Android ARM64 player builds passed; APK Java/Dex, signature and
16 KiB alignment checks passed. No new core semantic acceptance or actual Android
listening/provider run is claimed.
[EN](PLAYER_AUDIO_DIAGNOSTICS.md) / [RU](PLAYER_AUDIO_DIAGNOSTICS.ru.md).

## v0.585 Android device evidence

Samsung SM-S931B / Android16 user traces establish excessive AAudio queue drain,
not general Android compatibility. The manifest selects the blocking AudioTrack
backend. APK metadata/DEX checksums/signature/16 KiB alignment and identical native
payloads verified; native rebuild, runtime playback and the new driver's phone
acceptance remain pending. [Investigation](PLAYER_AUDIO_DIAGNOSTICS.md).

## v0.586 validation

Second real Android user archive: fixed AAudio overconsumption and normal source
tones, but incomplete 15-second PCM delivery. Native SDL bridge now exposes the
negotiated quantum. Nine focused host SDK/frontend checks and direct four-run
PCM trace integration use explicit dummy audio/video; these are not X11, physical
audio or Android execution. The regression also rejects the old zero-change flag.
Fresh Android ARM64 native/Java/Dex build, same v0.585 signer and 16 KiB ELF/ZIP
checks; actual new phone playback pending. No roadmap stage closes.

## v0.587 validation

Lean AGG/JIT Linux SDL Release with examples: 1225 passing tests and four explicit
platform/service skips out of 1229 registered. Generated executable permissions
were restored and the 44 initially unstarted cases all passed on rerun; no test
assertion failures remained. New direct/converted release, accepted-ring/retry
and allocation regressions also pass ASan/UBSan (leaks disabled). The allocation
fixture covers ordinary and nothrow allocation consistently. Strict C89 output
checks exercise both new APIs.

Real SDL dummy audio/video integration runs 01/03 in both modes, validates tone
amplitudes and absence of a large tone-to-silence edge in 01, and checks real
StopSounds control counts. This is not X11, hardware listening or an Android run.
Fresh NDK r27c ARM64/API23 native/SDL and JDK17/SDK35 Java/Dex APK build: signature
and 16 KiB alignment checked, persisted v0.585 signer reused. Natural source
endings and SWF files remain unchanged. Hardware release-boundary races and
physical click acceptance remain open. [Contracts](PLAYER_AUDIO_RELEASE.md).

## v0.588 phone confirmation / Подтверждение на телефоне

The user reports all three sound tests are now audibly correct on Samsung
SM-S931B/Android16 in interpreter/JIT. The latest six-run archive contains the
corrected ADPCM waveform: maximum adjacent output sample step in each 880 Hz
burst is 688 rather than the previous 1902. No software-starvation markers,
truncation or dropped diagnostic events appear. This is bounded acceptance of
these test movies, not general codec/device/endurance certification.
The archive's device.txt still says 0.587 because the Android menu/export label
was hard-coded; v0.589 reads the packaged version dynamically instead.

Пользователь подтвердил отсутствие щелчков в трёх звуковых тестах обоих режимов.
Это приёмка конкретных SWF на конкретном телефоне; она не закрывает ни одну
стадию. Старый номер в device.txt был ошибкой подписи отчёта, не основанием
выдавать его за повторный прогон старого звука.

## v0.589 keyboard polling / Опрос клавиатуры

Native per-context snapshot ABI, explicit transactional Key polling installation,
SDL virtual-key/English-ASCII translation and focus/background reset. Test 02 now
calls Key.isDown for arrows/Space and Key.getCode for its indicator. The C89
[example](../examples/embedding/keyboard_polling/README.md) demonstrates simultaneous
keys and reset. [EN contracts](KEYBOARD.md) / [RU](KEYBOARD.ru.md).
Listeners, clip/button key events, security origins, IME/text composition and
full reference/platform keyboard acceptance remain open. All nine stages remain
open; this advances input support in stages 3/5 and accompanies GC/JIT/platform
verification rather than claiming completion of the plan.

### v0.589 verification / Проверка

Lean AGG/JIT Linux SDL Release with examples: 1235 registered cases, 1231 passed
and four explicit native-macOS/native-Windows/SSIP skips after restoring missing
execute permissions and rerunning four unstarted cases. No assertion failures.
Final affected keyboard/API/C89 tutorial/SDL translation tests: 5/5. The three
core keyboard tests pass ASan/UBSan (LeakSanitizer disabled), including allocation
failure sweeps and SWF5/6 folded-name installation conflicts. Real SDL dummy
video/audio captures pass all three movies in both modes, including the updated
02's actual Key calls and preserved PCM tone levels. This is not native X11 or
physical keyboard acceptance. Android ARM64/API23 native build passes; Java/Dex
APK packaging uses the persisted signer. Phone keyboard/foreign-OS runs remain
open; the user's audio confirmation applies to the previous corrected assets.

## v0.590 Key listeners / Слушатели клавиатуры

Implemented explicit add/remove/dispatch APIs, strong registration roots, rooted
recipient snapshots, optional inherited/getter/script/native method dispatch and
ordered SDL transition buffering. Added Key.addListener/removeListener and a
read-only `_listeners` Array snapshot. Repeated SWF 02 loops keep one observer;
short taps reach both handlers before a movie tick.
[EN contracts](KEYBOARD_LISTENERS.md) / [RU](KEYBOARD_LISTENERS.ru.md),
[C89 tutorial](../examples/embedding/keyboard_listeners/README.md).

Реализованы регистрация/удаление/рассылка, GC-корни и снимок получателей;
изменения подписок действуют на следующее событие. Ошибки настоящих обработчиков
распространяются без отката уже выполненных действий. Это продвижение ввода,
GC и JIT, а не завершение стадий. Полные clip/button key events, security/IME,
точное поведение мутаций/идентичности Array оригинального Flash и аппаратная
приёмка платформ остаются открытыми. Все девять стадий открыты.

### v0.590 verification / Проверка

Lean AGG/JIT Linux SDL Release with examples: 1240 registered, 1236 passed and
four explicit native-macOS/native-Windows/SSIP skips after restoring executable
permissions for 28 initially unstarted cases and rerunning them successfully.
No assertion failures remained. Five core keyboard/snapshot/install/listener
regressions pass ASan/UBSan with LeakSanitizer disabled, including native C++
exception containment, actual GC during mutation and the 1024-listener bound.
The real SDL FIFO fixture also passes a separate strict-C89 ASan/UBSan build
linked to the same minimal-JIT library and actual SDL2, without opening devices.
The minimal NO_GRAPHICS CMake profile itself does not support the desktop target;
this separate fixture compilation is not a full sanitized desktop player build.

All three SDL dummy audio/video captures pass in interpreter/JIT, preserving
source tone amplitudes and corrected ADPCM boundaries. The shipped SWF 02 also
passes three loops and short taps in both modes with one observer and matching
counters. The C89 listener tutorial prints down/up and removed=1 deliveries=2.
Android ARM64/API23 native and Java/Dex APK builds pass, with the persisted test
signer, signature and 16 KiB ELF/ZIP alignment checked. No new physical keyboard,
Android playback or foreign-OS runtime acceptance is claimed. All nine stages
remain open.


## v0.591 explicit host keyboard focus / Выбор клипа хостом

Added [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L83) / [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L86) for one native
MovieClip, with a strong context root, private attachment validation and lazy
clearing when the clip or a physical ancestor is removed. Keyboard dispatch
snapshots the selected clip before Key listeners; callback selection changes
apply to the next event, including when a callback collects removed listeners.
The same clip in both channels deliberately receives two calls.
[EN contract](KEYBOARD_FOCUS.md) / [RU](KEYBOARD_FOCUS.ru.md).

Реализован явный C API выбора клипа, а не полный Selection: автоматический выбор
мышью/Tab, правила focusEnabled, Button/TextField и SWF ClipActions остаются
открытыми. Выбор хоста не сериализуется. Это продвижение стадий 3/5/7/8;
все девять стадий по-прежнему открыты.

### v0.591 verification / Проверка

Linux Release AGG/JIT/SDL with examples: 1241 registered, 1237 passed and four
explicit native-macOS/native-Windows/SSIP skips. Twenty-one initially unstarted
cached executables lost execute permissions; after full materialization and
permission restoration all 21 reruns pass. No runtime/assertion failures remain.

Four focused regressions pass ASan/UBSan with leak detection disabled: native
selection/GC/detached ancestry, listener mutation, dispatch allocation failure,
and real SWF6–8 interpreter/manual-JIT/automatic-JIT clip callbacks. The strict
C89 tutorial prints both listener events and both selected-clip events. The
Android ARM64/API23 NDK/SDL player builds and exports both new C ABI functions;
this is cross-build evidence, not device keyboard acceptance. RU/EN source links
and commented/no-assert tutorial checks pass. No new Windows/macOS runtime or
physical device acceptance is claimed.

## v0.592 Selection MovieClip subset / Скриптовый выбор клипа

Added explicit [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L101) with atomic publication and
versioned conflicts. Selection.setFocus accepts native clip objects or counted
dot/slash paths, applies focusEnabled/button-handler eligibility and revalidates
native attachment after getters. Selection.getFocus returns a counted native dot
path or null and shares the C host focus root. Trusted rooted entries permit GC
inside nested script getters without suspending enclosing ordinary native barriers.
[EN](SELECTION.md) / [RU](SELECTION.ru.md),
[C89 tutorial](../examples/embedding/selection/README.md).

Реализовано подмножество для хоста/SWF6–8; SWF1–5 не выдаются за поддержанный
текстовый Selection. SDL-плеер устанавливает namespace только для SWF6–8.
Поставляемый SWF 02 сам выбирает корень путём `_root`; отдельные root/Key-счётчики
сохраняются на трёх циклах в interpreter/JIT. Высота синего квадрата растёт при
отпусканиях выбранного корня; звуковые данные не менялись.

Открыты TextField/каретка/диапазоны, Button, Selection listeners и уведомления
onSetFocus/onKillFocus, автоматический выбор мышью/Tab, IME/рамки/security,
ClipActions, полная эталонная совместимость и платформенная приёмка. Все девять
стадий остаются открытыми.

### v0.592 focused verification / Целевые проверки

Linux Release AGG/JIT/SDL with examples: 1245 registered, 1241 passed and four
explicit native-macOS/native-Windows/SSIP skips. Nineteen cached executables lost
execute permissions; one bitmap allocation test exceeded its 30-second limit in
the parallel run. After permission restoration all 20 sequential reruns pass in
7.95 seconds total. No runtime/assertion failures remain. Six real SDL dummy
video/audio captures (three movies, interpreter/JIT) pass with peak 6400 and the
previous stereo amplitudes/ADPCM boundaries. This is not physical speaker testing.

Three new core regressions pass ASan/UBSan with leak detection disabled, including
installation/setFocus allocation failures, Unicode paths, real getter errors,
detachment during eligibility, versioned global conflicts, nested script GC and
SWF6–8 interpreter/manual/automatic-JIT execution. The actual interactive SWF
passes independent focus/listener counter checks over three loops in both modes.
The C89 tutorial has comments on every nonempty line and per-function guide/source
links; the catalog includes its grouped TOC and alphabetic glossary entry.

Android ARM64/API23 NDK/SDL player and Java/Dex APK build successfully. The APK
uses the persisted v0.585 test signer, with signature and 16 KiB native/ZIP alignment
checked; all three embedded SWFs match current source assets. The new C ABI symbol
is exported. This is build/runtime-fixture evidence, not physical keyboard or
new Android audio acceptance. No Windows/macOS device execution is claimed.

## v0.593 MovieClip focus notifications / Уведомления выбора клипа

Accepted script Selection identity changes now publish focus, deliver
onKillFocus(newClip), then onSetFocus(oldClip). Both endpoints remain rooted
through synchronous getters/callbacks, collection and recursive focus changes.
Nested changes persist; the outer endpoint snapshot still finishes delivery.
Missing/noncallable handlers are ignored without global-function fallback.
Actual handler errors propagate without rollback; C host setters remain silent.
[EN](SELECTION.md#focus-notifications) / [RU](SELECTION.ru.md#уведомления-фокуса).

Реализованы уведомления MovieClip, а не автоматический выбор касанием/Tab,
Selection listeners, TextField/Button, IME или полная эталонная совместимость.
Все девять этапов остаются открытыми. Результаты проверок приведены ниже.

### v0.593 verification / Проверка

Fresh Linux Release C89/C++98 AGG build with examples: 1257 registered,
1253 pass and four explicit skips (native macOS ARM64/Intel, Windows unwind,
external SSIP service). Thirteen cached test executables initially lacked execute
permission; after full byte materialization and permission restoration their
reruns pass. No assertion/runtime failure remains. The actual C89 tutorial prints
set/kill notifications with null opposite endpoints and clears focus successfully.

Three focused tests pass ASan/UBSan with leak detection disabled: endpoint
notification/reentry/GC, allocation-failure sweeps before and after publication,
and SWF6–8 interpreter/manual/automatic-JIT script callbacks collecting inside
notification. Source-link HTML/determinism and debugger tutorial checks pass.
This checkpoint supplies no new Android/Windows/macOS native execution or
reference-player ordering comparison; all roadmap stages remain open.

## v0.594 Selection observers / Наблюдатели смены фокуса

Selection.addListener/removeListener retain context-local object identities;
read-only _listeners returns independent native Array snapshots. Before publishing
focus, the transition prepares a rooted recipient snapshot. After clip callbacks,
observers receive old/new objects (or null). Mutation affects later transitions;
real errors stop delivery without rolling back committed focus. Registrations
survive public namespace replacement and are independent of Key.
[EN](SELECTION.md#selection-listeners) / [RU](SELECTION.ru.md#слушатели-selection).

Это ограниченная реализация: TextField/Button, выбор касанием/Tab, изменяемая
идентичность broadcaster-массива оригинального Flash, эталонный порядок и
новая нативная приёмка ОС не заявляются. Все девять стадий остаются открытыми.

### v0.594 verification / Проверка

Full Linux Release C89/C++98 AGG build with examples: 1259 registered,
1255 pass and four explicit native-macOS/native-Windows/SSIP skips. Cached
executables were fully materialized and execute permissions restored before
this run; there are no not-started tests or assertion/runtime failures.
The real C89 tutorial prints both clip events and both two-endpoint observer
events, clears focus and reports successful observer removal.

Five focused regressions pass ASan/UBSan with leak detection disabled: observer
identity/mutation/GC/isolation/limits, observer allocation sweeps, endpoint
notification/reentry/GC, installation/publication allocation sweeps and real
SWF6–8 interpreter/manual/automatic-JIT callbacks. The latter also throws from
an actual AVM1 observer and verifies committed focus plus restored GC/script/JIT
state. Source-link HTML/determinism, source-link freshness and debugger tutorial
checks pass. No new foreign native runtime/device/reference-player acceptance
is claimed; all nine roadmap stages remain open.

## v0.595 primary-pointer Mouse / События первичного указателя

Explicit native registration/delivery and the SWF6–8 Mouse namespace support
onMouseDown/onMouseMove/onMouseUp from host-supplied primary touch/stylus/mouse.
Rooted receiver snapshots precede pointer publication; active drag updates before
callback delivery. Recursive Mouse dispatch is rejected, while mutations affect
later events. Native registrations survive namespace replacement independently
of Key/Selection. [EN](MOUSE.md) / [RU](MOUSE.ru.md).

Курсор/колесо, direct MovieClip/Button/ClipActions, автоматический focus/hit route,
сырые касания, эталонный порядок и нативная/device/endurance приёмка не заявлены.
Все девять стадий остаются открытыми.

### v0.595 verification / Проверка

Linux Release C89/C++98 AGG build with examples: 1263 registered, 1259 pass and
four explicit native-macOS/native-Windows/SSIP skips. Eleven cached executables
were materialized with execute permission before this run. No runtime/assertion
or not-started failure remains. The strict-C89 tutorial prints all three event
positions and successful removal without requiring an OS input device.

Three new regressions pass ASan/UBSan with leak detection disabled: native Mouse
snapshot/mutation/GC/drag/recursion guards, installation/dispatch allocation
sweeps, and SWF6–8 interpreter/manual/automatic-JIT handlers including real GC and
an AVM1 exception. One initially cached sanitizer executable lacked execute
permission; the locally materialized executable passes directly. Source-link
freshness/HTML/determinism and debugger tutorial checks pass. No new native
Android/Windows/macOS run, touch device or original-player acceptance is claimed.
All roadmap stages remain open.

## v0.596 primary touch in player / Первичное касание плеера

The SDL host maps one real finger identity to Mouse down/move/up and ignores
synthetic duplicates/secondary fingers. Its shared native FIFO retains mixed
key/pointer order and event-time coordinates. Delivery restores stage origin;
focus reset cancels an old active batch without consuming callback-enqueued
new input. Explicit overflow/errors do not claim successful lost-event delivery.
[EN](PLAYER_POINTER.md) / [RU](PLAYER_POINTER.ru.md).

Проверки runtime библиотеки/очереди не заменяют физический touch/экран или
нативную Android/Windows/macOS приёмку. Полная scene/button маршрутизация,
multitouch, IME, колесо и курсор остаются открытыми; ни одна стадия не закрывается.

### v0.596 verification / Проверка

Fresh Linux Release C89/C++98 AGG core build with examples: 1263 registered,
1259 pass and four explicit native-macOS/native-Windows/SSIP skips. Separately,
a fresh lean AGG/JIT player built against static SDL2 2.30.0 passes six checks:
primary-contact/shared-FIFO runtime, existing keyboard/real-SWF keyboard fixtures,
options/help and unchanged audio assets. The pointer fixture uses explicit dummy
video, including actual SDL resize/letterbox filtering; no physical device is claimed.

Five focused regressions pass ASan/UBSan with leak detection disabled: pointer/
keyboard adapter tests plus the native Mouse, allocation-failure and real SWF6–8
interpreter/manual/automatic-JIT cases. SDL itself is an ordinary static Release
library, not an instrumented SDL build. Source links and tutorial checks pass.
The old SDL2 2.0.9 header lacks SDL_MOUSE_TOUCHID, so that newer SDK spelling is
conditionally referenced; no old-SDK runtime result is claimed. Foreign native
OS/device/endurance/reference acceptance and all nine stages remain open.

## v0.597 muted root audio / Явное отключение событийного звука

A real player run of the delivered SWFs exposed the old --mute route's missing
root StartSound callback. Explicit muted acknowledgements now permit timeline
playback without PCM decoding/queueing, a worker or an audio device. Cleanup
clears borrowed userdata before audio storage; stats count acknowledged events.
Normal sound-enabled error behavior and unsupported scene tags are not hidden.
[EN](PLAYER_POINTER.md#muted-movie-playback) /
[RU](PLAYER_POINTER.ru.md#ролики-без-звука).

This is player runtime integration, not codec/listening/device certification.
Ни один этап не закрыт; нативные foreign OS/device, полная Flash-совместимость
и итоговая приёмка по прежнему открыты.

### v0.597 verification / Проверка

The lean Linux AGG/JIT SDL2 2.30.0 player passes all eight registered frontend
checks. Muted interpreter and JIT checks each run all three delivered sound
movies for 120 frames with a deliberately invalid audio-driver name. They require
actual interpreter/native counters, acknowledged sound events and audio_device=0;
no unsupported-JIT skip was taken on this Linux x86-64 host. Other hosts without
a native backend explicitly skip only the separate JIT check with status 77.

Seven focused tests pass ASan/UBSan with leak detection disabled: native Mouse,
allocation sweeps, SWF6–8 interpreter/manual/automatic-JIT callbacks, pointer and
keyboard adapters, plus the two complete muted-player process checks. Initially
cached sanitizer executables lost permission/materialization; fresh relinking
and rerun pass all seven. SDL is an ordinary static Release library. Version
configuration/refresh checks pass separately. The complete unchanged core run is
recorded at v0.596 (1259 passes, four skips); no fresh full-core rerun is claimed
for this player-only correction. Source-link and tutorial checks pass. Native
foreign OS, physical touch/listening, general codecs and all stage acceptance
remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:36](../include/flashvm/mouse.h#L36) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
| `fvm_timers_next_delay` | function | [include/flashvm/timers.h:29](../include/flashvm/timers.h#L29) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:19](../include/flashvm/timers.h#L19) |
<!-- source-links:end -->
