# Controlled and background transfers

[Русский](BACKGROUND_TRANSFER.ru.md) · [Transport](SOCKET_TRANSPORT.md) · [Sources](PLAYER_SOURCES.md).

## Cancellation since v0.680

Original options/loader structures keep their ABI. [FVM_NetControl](../ports/network/socket.h#L21) in
[socket.h](../ports/network/socket.h) contains cancelled and borrowed userdata.
A null callback disables cancellation; nonzero requests it. Keep callback/state
alive until return. It runs on the transfer thread: it must be short, thread-safe,
nonthrowing, and must not enter VM/SDL UI or destroy provider state.

FVM_SocketHTTPRequest in [socket_http.h](../frontend/common/socket_http.h) wraps
unchanged socket options and control. fvm_desktop_socket_request_init initializes
all fields without startup (1 success, 0 null).
fvm_desktop_socket_fetch_controlled uses the ordinary fetch signature; userdata
is null for defaults or points to this request. fvm_desktop_http_fetch_controlled
in [http.h](../frontend/common/http.h) instead accepts null or [FVM_NetControl](../ports/network/socket.h#L21).
The original libcurl callback still rejects nonnull userdata.

Cancellation returns [FVM_ERROR](../include/flashvm/flashvm.h#L43) without publishing resolved. Sink errors retain
their exact result; discard partial bytes on every failure. Native waits check
cancellation in 50ms slices under the original total deadline. Buffered HTTP
and TLS I/O also check it. Libcurl uses its own progress/sink cadence and can be
slower. DNS, CA reads, crypto work and user callbacks cannot be forcibly interrupted.
Cancellation during DNS is observed after resolver return; this is not a hard
realtime completion guarantee. No thread is killed or library unloaded during
active execution. Cleanup balances every owned socket/Winsock/TLS reference.

Loopback regressions cancel before connect, a silent response, byte-drip HTTP and
TLS bodies. A separate libcurl probe exercises its real progress callback. These
are protocol tests, not complete Flash or native foreign OS/device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_NetControl` | struct | [ports/network/socket.h:21](../ports/network/socket.h#L21) |
<!-- source-links:end -->
