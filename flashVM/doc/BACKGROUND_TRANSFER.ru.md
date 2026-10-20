# Управляемые и фоновые загрузки

[English](BACKGROUND_TRANSFER.md) · [Транспорт](SOCKET_TRANSPORT.ru.md) · [Источники](PLAYER_SOURCES.ru.md).

## Отмена с v0.680

ABI прежних options/loader сохраняется. [FVM_NetControl](../ports/network/socket.h#L21) в
[socket.h](../ports/network/socket.h) содержит cancelled и заимствованный userdata.
Null callback отключает отмену, ненулевой результат запрашивает её. Сохраняйте
callback/состояние до возврата. Callback выполняется на потоке загрузки: он должен
быть коротким, thread-safe, без исключений, VM/SDL UI и удаления провайдера.

FVM_SocketHTTPRequest в [socket_http.h](../frontend/common/socket_http.h)
содержит неизменённые socket options и control.
fvm_desktop_socket_request_init инициализирует все поля без startup: 1 — успех,
0 — null. fvm_desktop_socket_fetch_controlled имеет обычную сигнатуру fetch;
userdata — null для defaults либо указатель request.
fvm_desktop_http_fetch_controlled из [http.h](../frontend/common/http.h)
принимает null либо [FVM_NetControl](../ports/network/socket.h#L21). Прежний libcurl callback по-прежнему
отклоняет ненулевой userdata.

Отмена даёт [FVM_ERROR](../include/flashvm/flashvm.h#L43) без публикации resolved. Ошибки sink возвращаются точно;
частичные данные при ошибке отбрасываются. Native waits проверяют отмену каждые
50ms ожидания под общим deadline; проверки есть и при разборе буферов HTTP/TLS.
Libcurl использует свою, потенциально более медленную progress/sink cadence.
DNS, чтение CA, crypto и пользовательский код принудительно не прерываются.
Отмена в DNS замечается после возврата resolver; жёсткой realtime-гарантии нет.
Потоки не убиваются, библиотеки не выгружаются во время работы. Собственные
ссылки socket/Winsock/TLS освобождаются обычным путём.

Loopback-тесты отменяют запрос до connect, молчащий ответ, byte-drip HTTP/TLS.
Отдельный libcurl probe проверяет его progress callback. Это не полный Flash API
и не native foreign OS/device acceptance.

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
