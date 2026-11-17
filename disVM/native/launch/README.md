# Styx terminal root before init / Styx-корень терминала перед init

[`styxlaunchprepare`](prepare.h) bridges an **already attached ordinary Styx
client** to the existing [pre-init namespace builder](../../runtime/LAUNCH-ENVIRONMENT.md).
It captures the attached root as a normal live VFS provider, then privately builds
the terminal-first union with the explicitly granted execution-provider root.
The returned namespace is owned by the caller and ready for the existing launch
handoff. Failure returns NULL without publishing a partial environment.

Это native SDK этап подготовки окружения, а не готовый сетевой запуск Dis.
Модуль не скачивается для исполнения на терминале. Корень терминала становится
обычным удалённым VFS-провайдером; сервер исполнения сохраняет свой выданный корень.
Пути по-прежнему разрешаются через VFS, независимо от того, файл это, устройство,
канал или сокет. Дерево не сериализуется и не фиксируется.

## FlashVM terminal and general requests / Терминал FlashVM и обычные запросы

This page's ordinary/bidir choice concerns VFS-root preparation. General services
may use either mode. The **FlashVM terminal connection requires bidir** for its
general two-way control/event/data/proxy traffic. The new
[request/ticket adapters](REQUESTS.md) expose a separate `styxbidirlaunchrequest`
constructor for that grant and reject ordinary clients. Input/output is only one
part of this terminal connection; additional bulk/stream channels remain supported
by the existing announcement architecture.

## API and ownership / API и владение

```c
/* options.provider and origins/policy were already authorized by the host.
 * options.terminal must be NULL; caller retains options.provider. */
DVM_Namespace *environment = styxlaunchprepare(terminal_client, &options,
    FVM_ACCESS_READ | FVM_ACCESS_EXEC);
/* terminal_client ref is consumed on both success and failure.
 * Check environment, pass it to the existing launch handoff, then release
 * the caller's namespace ref when ownership has been transferred/retired. */
```

The native SDK target is `styx_launch`, using the existing namespace core through
the `styx_namespace` target. Include [`prepare.h`](prepare.h) from its public include
path. The provider and options are borrowed until return; the adapter copies the
options, replaces only the local copy's terminal field, and retains roots through
ordinary provider/namespace capabilities. It consumes one client reference on
every outcome. Retain an additional client reference first if other native callers
must continue using that session independently.

Preflight checks reject a nonempty terminal field, missing provider, unknown
policy, invalid source roles/IDs, mismatched session associations and invalid
access ceilings before initiating metadata requests. READ+EXEC is required for
a browsable terminal root; WRITE is optional and never overrides remote rights.
An already requested cancellation also refuses before root acquisition. Current
native cancellation scopes flow through the ordinary rooted mount operations.
Identity strings are associations supplied by the trusted host, not credentials.

The mounted remote root is captured relative to the existing attach. There is no
dial, transport negotiation, root-name guessing, implicit host path, new access
authority or proxy selected by resource kind. Both roots must already be filtered
and authorized by their respective owners. The peer's normal attach/publication
preparation runs before this adapter receives the client.

## Ordinary by default, bidir explicitly / Обычный режим и явный bidir

An ordinary client/server connection is sufficient if the execution provider
already has a granted connection to the terminal export. Pass that client to
`styxlaunchprepare`. This function never creates another client/server, enables
P9MX, or acquires the physical carrier.

If a single carrier must also serve requests in the opposite direction, explicitly
create [`phybidircreate`](../../../remote_phy/mux/README.md) at both ends, match
the channel roles, start the existing original libstyx server owner, and create
the ordinary terminal client on the corresponding logical channel. Then call
this same adapter with that client. The server owner must already be running
before synchronous client construction/preparation starts. Grants in the two
sessions remain independent; opting into bidir grants no reverse VFS access.

The host retains the physical/bidir owner while the environment, opened handles
or other protocol users require it. Dropping the caller's client reference does
not detach the environment's retained root. Explicit transport shutdown still
fails remote operations normally; it does not retarget them or replay effects.
`phybidirfree` remains an explicit link abort, not a graceful delivery guarantee.

## Scope and limitations / Границы

The namespace builder retains the existing terminal-first data order and existing
provider-first code lookup policy. Provider provenance and per-file stack moves
use the normal namespace API; a copied namespace has an independent stack order.
Read/write/open permissions are decided by ordinary operations and fixed ceilings.

**Missing-name fallback requires an explicit error vocabulary.** Before retaining
or using the terminal client, the native host may select
`styxclienterrorprofile(client, STYX_ERRORS_INFERNO)` for a known Inferno-compatible
endpoint. The [lookup contract](../client/LOOKUP.md) then maps only exact Twalk
Enonexist to FVM_NOT_FOUND. Deep lookups walk one component per RPC to retain the
failure reason. A permission error, unknown text, failed Tstat, cancellation or
unacknowledged operation still stops lookup. Without opt-in, the default opaque
profile keeps the earlier conservative behavior. No wire extension is required.

Для известного Inferno endpoint host явно выбирает профиль до передачи client в
этот adapter. Подтверждённое отсутствие имени допускает нижний провайдер; отказ
поиска в каталоге и ошибка metadata после успешного walk — нет. Сам adapter
профиль не угадывает, новых прав и host fallback не создаёт.

The native [request/ticket bridge](REQUESTS.md) now connects captured roots to
existing provider prepare/take. No network launch request protocol, authenticated
transfer of arbitrary FD grants,
typed terminal init argument, provider-side guest startup or migration is added
here. Existing local launch tickets/terminal channels remain separate. This
adapter supplies the missing root-to-preparation step rather than claiming the
whole remote-execution pipeline is complete.

## Verified / Проверено

Three actual PTY scenarios exercise this adapter with original lib9/libstyx:
ordinary single session, two simultaneous bidir sessions and bidir with Tflush
cancellation while the opposite session delivers an event. PHY reads/writes are
fragmented to3/2bytes. No TCP, UDP or socketpair is used for these new scenarios.

Each scenario checks preflight/cancellation rejection, caller reference retention,
actual remote bytes through the prepared VFS, terminal/provider source positions,
per-file reorder, independent namespace copy, readonly ceiling, absence of host
fallback and local provider retirement. Existing ordinary/cancel/event tests still
run after the launch preparation, proving that namespace cleanup does not consume
the original independently held client.

All 328 Linux64 native SDK tests pass normally and under ASan/UBSan, with
LeakSanitizer disabled. Imported Inferno sources and FlashVM code are unchanged.

Additional explicit-profile ordinary/bidir cases verify missing-name fallback
against original libstyx, while remote search denial and failed stat never invoke
the lower provider. The original no-profile cases still check conservative denial.
