# Shared native resolver / Общий нативный резолвер

[Public declarations](../ports/network/resolver.h) ·
[Implementation](../ports/network/resolver.c) ·
[Dis Srv adapter](../../disVM/runtime/srv_adapter.c) ·
[Native tests](../../disVM/runtime/test_resolver.c)

The resolver port supplies synchronous OS name/service lookup without depending
on VM heaps, an Inferno network device or a GUI. It is shared host infrastructure,
not a Styx transport or a socket connection. All input strings are borrowed for
the call; result storage belongs to the caller. No native allocation escapes.
Separate callers use separate output storage and obey ordinary native resolver
restrictions. The Dis adapter additionally serializes calls with a real native
mutex while retaining production Srv VM-result construction and failure cleanup.

| API | Result and failure contract |
| --- | --- |
| `fvm_port_resolve_host(host, out)` | Resolve a nonempty host name/address into distinct numeric IPv4/IPv6 strings. `out` changes only on [`FVM_OK`](../include/flashvm/flashvm.h#L44); invalid input or failed resolution returns an error. |
| `fvm_port_resolve_address(address, out)` | Require a numeric input and a reverse host-name result. Failure preserves `out`; a fabricated numeric fallback is not returned as a host name. |
| `fvm_port_resolve_service(network, service, port)` | Accept tcp/udp and a decimal port or native service name. Decimal values must be 0..65535. Unsupported networks, invalid input and lookup failures preserve `port`. |

[`FVM_ResolverNames`](../ports/network/resolver.h#L17) contains at most [`FVM_RESOLVER_NAMES`](../ports/network/resolver.h#L6) answers, each bounded
by [`FVM_RESOLVER_TEXT`](../ports/network/resolver.h#L7), with an initialized count. Duplicate addresses are removed
without reordering the surviving results. Reverse lookup exposes the native
resolver's returned names, not an exhaustive alias database. Unicode/IDNA policy
is not added by this adapter. The accepted runtime tests use numeric loopback,
checked decimal services and malformed inputs; no external DNS or connection is
needed. Native reverse/service databases and physical Windows/macOS/Android
execution are not established by those Linux AMD64 checks.

All three calls may block in native resolver work; there is no timeout or
cancellation promise. Windows calls acquire/release a balanced temporary Winsock
reference where native lookup is required. The API never closes caller sockets
or standard descriptors. The current Dis owner remains synchronous during DNS.
No runtime or GUI callback is delivered from this C port.

Резолвер использует нативные функции ОС, не переносит сетевую подсистему Inferno
и не открывает соединение. Аргументы заимствуются на время вызова, память
результата принадлежит вызывающему коду. При ошибке выходные значения не меняются.
Прямой поиск возвращает числовые адреса, обратный требует настоящее имя хоста,
поиск порта принимает tcp/udp и проверяет диапазон 0..65535 до преобразования.

Вызовы синхронные: ограничения времени и отмены пока нет. Srv использует общий
переходник с настоящим нативным mutex и upstream-механизмом защиты GC-корней и
освобождения результатов при ошибках. Тесты Linux AMD64 не объявляют готовыми
сетевой Styx-сервер, namespace, Unicode/IDNA либо остальные платформы.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_port_resolve_address` | function | [ports/network/resolver.c:127](../ports/network/resolver.c#L127) |
| `fvm_port_resolve_host` | function | [ports/network/resolver.c:120](../ports/network/resolver.c#L120) |
| `fvm_port_resolve_service` | function | [ports/network/resolver.c:135](../ports/network/resolver.c#L135) |
| `FVM_RESOLVER_NAMES` | macro / define | [ports/network/resolver.h:6](../ports/network/resolver.h#L6) |
| `FVM_RESOLVER_TEXT` | macro / define | [ports/network/resolver.h:7](../ports/network/resolver.h#L7) |
| `FVM_ResolverNames` | struct | [ports/network/resolver.h:17](../ports/network/resolver.h#L17) |
<!-- source-links:end -->
