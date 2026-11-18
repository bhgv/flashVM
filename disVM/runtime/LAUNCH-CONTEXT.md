# Accepted launch data / Данные принятого запуска

[`Launchcontext`](../module/launchcontext.m) is a DisVM extension exposing the
current process's copied logical module label and opaque startup bytes. It closes
the gap between native request acceptance and a bootstrap's ordinary typed
application call. It works without a FlashVM/display context and does not change
Inferno's standard Sys interface or inject arguments into native Dis frames.

`Launchcontext` — расширение DisVM: общий bootstrap получает данные принятого
запуска, а затем вызывает приложение по выбранному обычному `.m`-интерфейсу.
Графический контекст не обязателен; консольные модули продолжают пользоваться FD,
включая stdin/stdout/stderr. Новый API не выбирает место исполнения приложения.

## Interface / Интерфейс

| Operation | Result |
| --- | --- |
| `load Launchcontext Launchcontext->PATH` | Matched builtin `$Launchcontext` |
| `name()` | Fresh managed string containing the accepted logical module label; nil without an accepted handoff |
| `arguments()` | Fresh mutable byte array, bounded by65536; nil for absent/empty startup bytes |

The nonempty `name()` distinguishes an accepted launch with empty arguments from
ordinary startup without a handoff. The module label remains descriptive: it is
not a credential, host-path fallback, executable capability or implicit remote
call. A bootstrap interprets it only under the accepted launch's VFS and its own
selected typed interface. The API performs no provider I/O or namespace lookup.

```limbo
include "launchcontext.m";

# Inside the already accepted bootstrap's ordinary init:
launch := load Launchcontext Launchcontext->PATH;
name := launch->name();
bytes := launch->arguments();
# Decode bytes according to the caller/bootstrap agreement, then use the
# application's existing typed interface. No argument encoding is imposed here.
```

Payloads are binary, including embedded zero and bytes128..255. No string/list
encoding, native pointer transport, FD encoding or automatic argument decoder is
introduced. Repeated calls return independently owned results; changing one array
cannot modify native startup data, another result or another process's result.
The existing guarded result replacement retires the previous managed owner.

## Constructor ownership / Владение constructor

The trusted host still supplies the previously loaded zero-argument bootstrap to
`dvm_runtime_launch_module`. During unpublished-process adoption the runtime
validates label termination, payload bound and non-NULL storage for nonempty data,
then allocates one private native snapshot. Failure leaves the handoff and private
process environment unchanged. No successful snapshot is installed until all
fallible preparation and the optional Unibus claim have succeeded.

A successful adoption retains its independent snapshot even after the host
changes/frees the handoff metadata and the requester/provider wrappers retire.
Each child constructor retains the immutable snapshot, including display-free
children. A failed child setup rolls that pin back. Owner retirement and failed
publication drop their native pins; the final release frees native storage without
provider callbacks, managed references or I/O. Refcount overflow refuses creation
of another owner without wrapping or resurrecting a zero-reference allocation.

Snapshot здесь относится только к неизменяемым **стартовым байтам запроса**.
VFS и её дерево остаются живыми. Наследование startup data не закрепляет namespace:
каждый поток по-прежнему может иметь свой разрешённый bind/union-набор ресурсов.
API не приостанавливает guest и не требует cooperative обслуживания.

## Execution location / Место исполнения

A process may call an accessible module belonging to the terminal, its provider
or a third system, as selected by its configured VFS. The called module must run
where it belongs, with its prepared environment, dependencies and devices. The
caller and callee roles may therefore reverse or form a chain; they are not a
one-way permission hierarchy. Namespace accessibility and provider permissions
remain authoritative.

Процесс провайдера может вызывать доступные через его VFS Dis-модули клиента и
других машин; клиент может вызывать провайдера. Bind/union задают видимость и выбор,
а межпроцессная/межмашинная маршрутизация должна сохранять место выполнения
выбранного модуля. `Launchcontext` не вводит запрет по system ID или роли источника.

This change supplies accepted startup data only. It does not yet connect ordinary
`load` to transparent remote typed module calls, supply a network launch listener,
encode remote capabilities or implement live migration. The current
[provider-backed reader](PROVIDER-MODULES.md) is an internal image-acquisition stage
inside an already selected execution runtime, not evidence of remote-call routing.
The existing plain-terminal local-reader refusal is an implementation boundary
until remote dispatch is connected, not the intended module-access policy.

## Actual verification / Реальные проверки

The dedicated interpreter and JIT run eight new executions: ordinary startup
without a handoff and accepted launches with257 binary bytes, zero bytes and the
maximum65536 bytes. Four concurrent child owners repeatedly obtain independent
copies; parent and children mutate returned arrays without changing fresh copies.
Native checks validate the entire snapshot; managed checks cover lengths and
binary positions including the final byte of maximum-size data.

The accepted bootstrap has no FlashVM grant and exercises real stdin/stdout/stderr.
The native fixture poisons/frees the request input and handoff input independently.
Before publication it rejects oversized payloads, missing nonempty payload storage,
unterminated labels and an actual snapshot malloc failure, then successfully
reuses the preserved handoff. A native overflow check covers unpublished refcount
saturation. Actual snapshot free observation and root destruction verify complete
native retirement after child killwait and shutdown.

The existing provider-module bootstrap also obtains its application's label
and binary startup data through this builtin before the ordinary typed load/call.
Its36 executions retain the separate native blocked-reader killwait cases.
Both sets use source-built native-cell code and the supported dedicated OS-thread
model. ASan/UBSan runs use a fully instrumented runtime; LeakSanitizer remains
disabled in this restricted environment. No executing FlashVM or completed
cross-machine module-call path is claimed by these fixtures.
