# Встроенный сокетный транспорт плеера

[English](SOCKET_TRANSPORT.md) · [Источники и загрузка](PLAYER_SOURCES.ru.md) ·
[Сборка и cross-build](DESKTOP_FRONTEND.ru.md) · [Пример](../examples/player/socket_loading/README.ru.md).

С v0.677 HTTP загружается напрямую через TCP POSIX/Winsock без libcurl.
Сетевой старт, дочерние SWF, замена root и loadVariables используют один callback
и собственный canonical origin каждого фильма. Встроенный SWF может использовать
абсолютный URL или явно заданный сетевой origin. Это не XMLSocket, не модель
security sandbox, не асинхронная/progressive загрузка и не полный Flash networking.

## Сборка и выбор транспорта

```sh
cmake -S flashVM -B build-socket -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_HTTP=ON
cmake --build build-socket --target flashvm_player flashvm_example_socket_loading
./build-socket/frontend/flashvm_player --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

Для нестандартного SDK задайте SDL2_DIR. FVM_PLAYER_HTTP включает libcurl HTTP(S),
FVM_PLAYER_SOCKET_HTTP — встроенный транспорт. Одновременно включить оба нельзя:
CMake выдаёт ошибку. Оба по умолчанию OFF; ядро VM не зависит от них или SDL.
Хост может подменить loader.fetch своим callback. Отдельная статическая цель
flashvm_socket_transport предоставляет callback без запуска VM/SDL.

При FVM_PLAYER_HOST_SHARED=ON транспорт остаётся PIC static-зависимостью;
CMake target передаёт её потребителям C/C++. Windows дополнительно линкует
ws2_32. Каждый запрос владеет своей парой WSAStartup/WSACleanup и не завершает
чужое владение Winsock. Глобальный обработчик SIGPIPE не устанавливается.

Для cross-build используйте [explicit toolchain](../../toolchains/explicit.cmake),
FVM_TARGET_SYSTEM, FVM_TARGET_PROCESSOR, FVM_TARGET_CC/FVM_TARGET_CXX и SDL2_DIR
целевой платформы; полные команды SDK приведены в desktop guide. Сокеты используют
целевые системные заголовки/библиотеки. Сборка Linux ARM64/RISC-V64 и Windows
x86_64 не заменяет запуск. macOS/Android требуют настоящих SDK и проверки работы.
Android также требует INTERNET в manifest и Activity/surface-хост; политика
cleartext-соединений задаётся приложением.

## C API

Публичный callback: [socket_http.h](../frontend/common/socket_http.h).
Политика загрузки: [source.h](../frontend/common/source.h).

| API | Контракт |
| --- | --- |
| [fvm_desktop_socket_options_init](../frontend/common/socket_http.h#L32) | В структуру FVM_SocketHTTPOptions вызывающей программы записывает connect=10000ms, total=30000ms. Возвращает 1; null даёт 0. Нет I/O/аллокаций/сохранения указателей. |
| [fvm_desktop_socket_fetch](../frontend/common/socket_http.h#L39) | Обязательны reference, write, resolved и ненулевая capacity. base необязателен для абсолютного URL. userdata — null для defaults или заимствованные options. Синхронно выполняет GET, передаёт бинарные чанки write(bytes,size,write_userdata). Canonical origin с NUL публикуется в resolved только при успехе. |

Настройки: 0 < connect_ms <= total_ms <= 120000. DNS выполняется системным
синхронным getaddrinfo: затраченное время учитывается, но сам resolver нельзя
прервать этим API. Неблокирующие connect/send/read и readiness используют общий
монотонный deadline, без сброса на каждом байте. Код sink также нельзя прервать.
Поэтому callback целиком может занять больше total_ms при DNS или медленном sink.
Асинхронный resolver/cancel — отдельная работа.

[FVM_OK](../include/flashvm/flashvm.h#L42) означает завершение framing и доставки. [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) — неверные
аргументы/лимиты, малая capacity или превышение тела. [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) — HTTPS без
TLS-сборки. [FVM_ERROR](../include/flashvm/flashvm.h#L43) — ошибка TCP/I/O/deadline/status/framing; отказ malloc
в конструкторе платформенного сокета также пока отображается в [FVM_ERROR](../include/flashvm/flashvm.h#L43).
Ошибка sink возвращается без изменения, новые callback после неё не вызываются.
Перед поздней ошибкой sink мог получить часть тела: её надо отбросить при любом
не-OK. Аккумулятор плеера уже делает это. resolved при ошибке не изменяется.

Строки, options и userdata заимствованы до возврата. Все собственные сокеты и
буферы освобождаются на любом пути. VM-объекты, GC roots и скрытая регистрация
в транспорте отсутствуют. В плеере callback работает на безопасной границе
основного UI/VM-потока: нельзя рекурсивно запускать плеер, вызывать VM/SDL или
удалять состояние хоста. Внешние эффекты пользовательского sink не откатываются.

## Протокол и лимиты

Принимаются escaped ASCII HTTP(S), DNS/IPv4/IPv6 в скобках, порты 1..65535,
относительные пути, network-path и query. Удаляются literal dot segments и
fragment; scheme/host переводятся в нижний регистр, числовой порт нормализуется,
default port не печатается. Percent escapes, включая точки/NUL, не декодируются.
Отклоняются credentials, пробелы/control/backslash, повреждённые escapes,
неэкранированный non-ASCII, другие схемы и scoped IPv6. IDN задаётся в ASCII IDNA,
пути — percent-encoded. Это ограниченный HTTP URI resolver.

Запрос: HTTP/1.1 GET с Host, Connection: close и Accept-Encoding: identity.
Ответы: HTTP/1.0/1.1, финальные 2xx, ограниченные interim кроме 101,
Content-Length, chunked с проверкой extensions/trailers или завершение по EOF.
204/205 не допускают ненулевую длину и chunk coding. При известном framing конец
соединения не ожидается. Нет connection reuse, redirects, proxies, cookies,
authentication, POST, автоматической отправки переменных и декодирования gzip.

Строго отклоняются duplicate Content-Length (даже одинаковые), комбинация CL/TE,
другие/множественные transfer codings, non-identity content encoding, folded
fields, bare LF, запрещённые control, пробел перед colon и framing в trailers.
Это может отвергнуть permissive-сервер, но не переинтерпретирует неоднозначность.
Лимиты: URI 4096 байт, строка 8192, все status/header/trailer 32768, 256 полей,
8 interim, 65536 чанков, 64MiB decoded body. Слишком большая объявленная длина
отклоняется до выдачи тела. Обрыв до framed end — ошибка, даже если частичные
байты уже похожи на валидный SWF.

## Проверки и незакрытая приёмка

Независимый raw loopback TCP server проверяет binary NUL, дробные записи,
правильные/повреждённые framing/trailers, лимиты, точную ошибку sink и общий
deadline при медленных байтах/молчании сервера. URI fixture содержит примеры
RFC3986 и повреждённые адреса. Плеер исполняет реальные root/child/form SWF в
interpreter/JIT через этот транспорт.

```sh
cmake --build build-socket --target flashvm_socket_url flashvm_socket_client flashvm_frontend_memory_origin
ctest --test-dir build-socket -R 'socket_|player_http|player_child_loading' --output-on-failure
```

Loopback/dummy-display не подтверждают Internet, foreign OS, физическое окно или
полную совместимость Flash. Девять этапов остаются открытыми. Прерывание DNS и асинхронность требуют отдельной реализации/приёмки.
## Проверяемый HTTPS с v0.678

Включите FVM_PLAYER_SOCKET_TLS вместе с FVM_PLAYER_SOCKET_HTTP. Нужен SDK
OpenSSL 1.1.1+ (проверен 3.0). Всегда включены SSL_VERIFY_PEER, проверка цепочки,
DNS/IP, DNS SNI и минимум TLS1.2; выключить проверку нельзя. Custom BIO направляет
все зашифрованные данные через тот же TCP-адаптер с общим deadline и локальным
подавлением SIGPIPE. TLS1.2 и TLS1.3 проверяются отдельно.

```sh
cmake -S flashVM -B build-socket -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_TLS=ON -DOPENSSL_USE_STATIC_LIBS=TRUE
cmake --build build-socket --target flashvm_player flashvm_example_socket_loading
./build-socket/frontend/flashvm_example_socket_loading https://localhost:8443/start.swf /absolute/test-ca.pem
```

Предполагается исходная конфигурация player/SDL/render из первой команды сборки.
OPENSSL_USE_STATIC_LIBS выбирает статические архивы целевого OpenSSL, но не делает
весь executable полностью статическим. Для cross-build задайте OPENSSL_ROOT_DIR,
целевые find-root/sysroot CMake. Нельзя подключать host OpenSSL к другой платформе.
При TLS OFF OpenSSL не нужен; HTTPS даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) до DNS/TCP. TLS ON без
SOCKET_HTTP — ошибка конфигурации.

В FVM_SocketHTTPOptions добавлен ca_file, заимствованный до возврата fetch.
Всегда используйте fvm_desktop_socket_options_init: он записывает null.
Null выбирает default trust OpenSSL, включая SSL_CERT_FILE/SSL_CERT_DIR.
Ненулевой путь выбирает только указанный PEM CA bundle для данного запроса.
Ошибочный/пустой/нечитаемый CA и отказ проверки дают [FVM_ERROR](../include/flashvm/flashvm.h#L43) до HTTP payload.
Кодировка пути определяется SDK OpenSSL, а не SDL UTF-8 adapter. CA относится
к файловой системе хоста, не к URL фильма. Не принимайте CA override от
недоверенного SWF. Глобального setter нет. Windows/Android-приложению может
понадобиться поставляемый PEM bundle: default OpenSSL paths не означают
автоматическое использование native certificate store.

Настройку TLS/чтение CA нельзя прервать синхронным API, как DNS/sink. Elapsed
проверяется до/после handshake и во всех native I/O. Контекст, сеанс и BIO
независимы для запроса и освобождаются перед сокетом. Нет OPENSSL_cleanup и
глобальной смены trust. Очередь ошибок OpenSSL вызывающего потока очищается перед
TLS-операциями и может содержать транспортную диагностику после возврата;
свои ошибки хост должен обработать заранее. Другие потоки не затрагиваются.

Known-length/chunked HTTPS завершается на границе сообщения без ожидания shutdown.
EOF-framed HTTPS требует close_notify; голый TCP EOF — ошибка, в том числе при
SSL_ERROR_SYSCALL старого OpenSSL. Очистка не выполняет дополнительный блокирующий
SSL_shutdown. Нет session cache, client certificates, proxy, загрузки revocation,
pinning API и HTTP/2. Проверки следуют trust/policy выбранного OpenSSL SDK.

Тесты создают временный CA и valid/wrong-name/expired сертификаты: DNS/IP/SNI,
private/default trust, отказ недоверенному серверу, TLS1.2/1.3, чистый/оборванный
EOF, медленный body/handshake, точная ошибка sink, HTTPS child/form в interpreter/JIT
и исполняемый C-хост. Закрытые ключи удаляются и не коммитятся. CMake регистрирует
тест при наличии executable openssl хоста, native tests и examples.
Transport probe дополнительно проверен UBSan в Linux; OpenSSL и остальная VM
этим не инструментированы. TLS с foreign SDK ещё не проверен сборкой/запуском;
успех cross-build обычного HTTP не означает foreign TLS-приёмку.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
