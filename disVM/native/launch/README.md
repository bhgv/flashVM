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

**Missing-name fallback over Styx is still conservative.** The current generic
mount adapter maps remote path failures to FVM_ERROR, not a trustworthy
FVM_NOT_FOUND. The union therefore refuses instead of trying a lower layer when
it cannot distinguish absence from denial/failure. This change does not classify
arbitrary Rerror strings as absence. Exact remote absence reporting needs a
separate design/implementation before claiming full missing-path fallback for
these network roots. Explicit provider selection/reordering already works.

Нельзя считать любой Rerror отсутствием: это может быть отказ доступа или сбой.
Пока надёжное различение не проведено, нижний слой автоматически не выбирается.
Это ограничение существующего Styx mount, а не снимок или замена namespace.

No network launch request protocol, authenticated transfer of arbitrary FD grants,
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

All 299 Linux64 native SDK tests pass normally and under ASan/UBSan, with
LeakSanitizer disabled. Imported Inferno sources and FlashVM code are unchanged.
