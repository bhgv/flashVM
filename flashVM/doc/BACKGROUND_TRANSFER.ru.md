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
## Фоновые задания C89 с v0.681

[transfer.h](../frontend/common/transfer.h) предоставляет opaque
FVM_DesktopTransfer и структуры FVM_DesktopTransferOptions/View.
fvm_desktop_transfer_options_init задаёт max_bytes=64MiB без провайдера: 1 — успех,
0 — null. Нужен worker-safe FVM_DesktopTransferFetch и необязательный immutable
userdata; callback расширяет обычный fetch явным cancellation control. VM-объекты,
managed strings и UI-объекты не передаются потоку. Импорт байтов выполняет owner.

fvm_desktop_transfer_begin(reference,base,options,out) копирует ограниченные строки
и options, резервирует один из 64 живых handles этого SDK и запускает SDL worker.
Userdata и вложенные строки/CA заимствованы до release. max_bytes: 1..64MiB.
Ошибки: INVALID_ARGUMENT, ERROR при квоте, OUT_OF_MEMORY при allocation/thread.
*out при ошибке не меняется. Video/audio не инициализируются; provider/network
не выполняются на вызывающем потоке.

fvm_desktop_transfer_poll копирует view под mutex: OK либо INVALID_ARGUMENT для
null. PENDING: result=NOT_FOUND, изменяемые байты/адрес не выдаются. DONE:
immutable counted bytes (допустимо пустое тело) и непустой provenance, заимствованы
до release. FAILED: точная первая ошибка sink/provider без payload. CANCELLED:
ERROR без payload. received сохраняет накопленный progress после удаления
ошибочного/отменённого буфера. Игнорирование ошибки sink провайдером не превращается
в успех. Пустой/неограниченный NUL provenance — ошибка.

fvm_desktop_transfer_cancel возвращает 1 для pending, 0 для terminal/null;
не блокирует, не освобождает storage и не отзывает готовый view. Отмена до
terminal publication побеждает поздний успех provider.
fvm_desktop_transfer_release отменяет, join и освобождает на создавшем потоке:
OK, null также OK, другой поток INVALID_ARGUMENT. Некоперативный provider/DNS
может задержать join. Нет detach/forced kill. Нельзя одновременно release и
poll/cancel или выгружать библиотеку с активным worker. Все borrowed данные
сохраняются до возврата release.

fvm_desktop_transfer_active считает живые handles, включая готовые до release.
fvm_desktop_shutdown отказывает, пока хотя бы один жив, даже без video/audio.
Задания освобождаются до завершения соответствующего SDK; глобального setter
провайдера нет. fvm_desktop_socket_transfer_fetch принимает native options либо
null. fvm_desktop_http_transfer_fetch требует null userdata и libcurl с флагом
CURL_VERSION_THREADSAFE; старые global init/cleanup возвращают UNSUPPORTED.
Синхронный API сохраняет прежние SDK и правила координации.

Детерминированные тесты: копии строк, разные owner/worker, binary NUL, пустое тело,
pending view, поздняя отмена, ошибки sink/provider, игнорируемый sink failure,
неверный provenance, wrong-thread release, квота 64 и shutdown до/после join.
Сетевые тесты отмены отдельны. Это не TSan/leak/foreign OS acceptance.

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
