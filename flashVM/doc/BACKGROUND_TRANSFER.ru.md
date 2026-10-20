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
[fvm_desktop_socket_request_init](../frontend/common/socket_http.h#L55) инициализирует все поля без startup: 1 — успех,
0 — null. [fvm_desktop_socket_fetch_controlled](../frontend/common/socket_http.h#L59) имеет обычную сигнатуру fetch;
userdata — null для defaults либо указатель request.
[fvm_desktop_http_fetch_controlled](../frontend/common/http.h#L29) из [http.h](../frontend/common/http.h)
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
[FVM_DesktopTransfer](../frontend/common/transfer.h#L34) и структуры [FVM_DesktopTransferOptions](../frontend/common/transfer.h#L27)/View.
[fvm_desktop_transfer_options_init](../frontend/common/transfer.h#L47) задаёт max_bytes=64MiB без провайдера: 1 — успех,
0 — null. Нужен worker-safe [FVM_DesktopTransferFetch](../frontend/common/transfer.h#L21) и необязательный immutable
userdata; callback расширяет обычный fetch явным cancellation control. VM-объекты,
managed strings и UI-объекты не передаются потоку. Импорт байтов выполняет owner.

[fvm_desktop_transfer_begin](../frontend/common/transfer.h#L51)(reference,base,options,out) копирует ограниченные строки
и options, резервирует один из 64 живых handles этого SDK и запускает SDL worker.
Userdata и вложенные строки/CA заимствованы до release. max_bytes: 1..64MiB.
Ошибки: INVALID_ARGUMENT, ERROR при квоте, OUT_OF_MEMORY при allocation/thread.
*out при ошибке не меняется. Video/audio не инициализируются; provider/network
не выполняются на вызывающем потоке.

[fvm_desktop_transfer_poll](../frontend/common/transfer.h#L55) копирует view под mutex: OK либо INVALID_ARGUMENT для
null. PENDING: result=NOT_FOUND, изменяемые байты/адрес не выдаются. DONE:
immutable counted bytes (допустимо пустое тело) и непустой provenance, заимствованы
до release. FAILED: точная первая ошибка sink/provider без payload. CANCELLED:
ERROR без payload. received сохраняет накопленный progress после удаления
ошибочного/отменённого буфера. Игнорирование ошибки sink провайдером не превращается
в успех. Пустой/неограниченный NUL provenance — ошибка.

[fvm_desktop_transfer_cancel](../frontend/common/transfer.h#L59) возвращает 1 для pending, 0 для terminal/null;
не блокирует, не освобождает storage и не отзывает готовый view. Отмена до
terminal publication побеждает поздний успех provider.
[fvm_desktop_transfer_release](../frontend/common/transfer.h#L63) отменяет, join и освобождает на создавшем потоке:
OK, null также OK, другой поток INVALID_ARGUMENT. Некоперативный provider/DNS
может задержать join. Нет detach/forced kill. Нельзя одновременно release и
poll/cancel или выгружать библиотеку с активным worker. Все borrowed данные
сохраняются до возврата release.

[fvm_desktop_transfer_active](../frontend/common/transfer.h#L65) считает живые handles, включая готовые до release.
[fvm_desktop_shutdown](../frontend/common/player.h#L49) отказывает, пока хотя бы один жив, даже без video/audio.
Задания освобождаются до завершения соответствующего SDK; глобального setter
провайдера нет. [fvm_desktop_socket_transfer_fetch](../frontend/common/socket_http.h#L64) принимает native options либо
null. [fvm_desktop_http_transfer_fetch](../frontend/common/http.h#L34) требует null userdata и libcurl с флагом
CURL_VERSION_THREADSAFE; старые global init/cleanup возвращают UNSUPPORTED.
Синхронный API сохраняет прежние SDK и правила координации.

Детерминированные тесты: копии строк, разные owner/worker, binary NUL, пустое тело,
pending view, поздняя отмена, ошибки sink/provider, игнорируемый sink failure,
неверный provenance, wrong-thread release, квота 64 и shutdown до/после join.
Сетевые тесты отмены отдельны. Это не TSan/leak/foreign OS acceptance.
## Интеграция плеера с v0.682

Явный режим: `--background-loads` либо новые C API из
[background.h](../frontend/common/background.h). Прежние синхронные API и layout
options/source/loader сохраняются. Старые callbacks не переносятся в другой поток
неявно. CLI переводит флаг в [fvm_desktop_run_background](../frontend/common/background.h#L34); прежний scalar-options
parser не имеет поля для него. IDE вызывает background runner либо job API.

```sh
./build-socket/frontend/flashvm_player --background-loads --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

[fvm_desktop_background_loader_init](../frontend/common/background.h#L21) задаёт [FVM_DesktopBackgroundLoader](../frontend/common/background.h#L13):
transfer defaults без провайдера, одну загрузку, 1 — успех, 0 — null, без startup.
[fvm_desktop_run_source_background](../frontend/common/background.h#L28) принимает обычные options/source, обязательный
background loader и необязательный static error out. Нужны worker-safe fetch,
max_bytes=1..64MiB, max_loads=1..1024. Loader, URL/options/memory input и вложенный
immutable userdata заимствованы до синхронного возврата. Коды: 0 — успех/закрытие,
1 — playback/provider failure, 2 — неверные аргументы. Неверная policy отклоняется
до worker/VM/window. Частичные ошибочные данные не импортируются. Import/setters/GC
остаются на основном потоке.

[fvm_desktop_run_background](../frontend/common/background.h#L34) обслуживает playlist с выбранным native/thread-safe
libcurl provider. Без провайдера local playback использует обычный runner, URL
остаётся unsupported. Дубликат CLI-флага отклоняется; после `--` он становится
буквальным именем файла. Help/version не запускают network/file/VM/display.
Режим включается явно и не меняет default network mode.

Во время child/form network wait owner обрабатывает ограниченные native events,
перерисовывает сохранённый кадр при expose/resize, сохраняет input snapshots и
согласует foreground/background audio. Script/timers и import ждут worker.
Timeline приостановлен на безопасной границе: это отзывчивый native UI с фоновым
провайдером, а не полная async Flash resource/timeline semantics. Сохраняются
лимиты input FIFO и обработка переполнения. Progressive SWF decoding не добавлен.

Закрытие активного view отменяет/join загрузку, отбрасывает частичный ресурс,
освобождает GC pins всей очереди и завершает playlist со статусом 0. Native/provider
ошибки по-прежнему дают playback failure. Worker не получает VM/window pointers.
Во время startup и полной root-замены активного player view нет: fetch фоновый,
но встроенного progress window/startup close control нет. Filesystem читается на
owner. DNS/некооперативный код может задержать join, и runner не возвращается до
завершения workers. Для собственного startup UI используйте begin/poll/cancel/
release в event loop хоста, сохраняя borrowed данные до возврата release.

[Подробный C89-пример](../examples/player/background_loading/README.ru.md) явно
выбирает callback. Тест вводит настоящий SDL close из другого потока после первых
байтов child: отмена до завершения сервера, нет второго queued fetch, cleanup
успешен и живых jobs нет. Отдельно проверены normal/background HTTP/HTTPS
startup/child/form в interpreter/JIT и C executable. Это не foreign OS native window.

Итог серии: 86 выбранных native tests, 11 shared native-TLS, 9 shared libcurl,
3 no-provider metadata/job checks. Transfer implementation прошла UBSan со связанным
настоящим SDK/shutdown fixture; SDL/VM/crypto этим не инструментированы. Linux
ARM64/RISC-V64 и Windows x86_64 player/example слинкованы; Windows DLL C/C++98
consumers/transfer fixtures слинкованы, без запуска. Полная async loading,
прерывание DNS, foreign runtime/device и итоговая приёмка этапа 9 остаются открыты.

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
