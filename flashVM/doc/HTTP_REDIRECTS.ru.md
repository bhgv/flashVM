# Ограниченные HTTP GET перенаправления (v0.689)

[English](HTTP_REDIRECTS.md) · [Socket-транспорт](SOCKET_TRANSPORT.ru.md) ·
[Наблюдение загрузок](LOAD_MONITOR.ru.md).

[fvm_desktop_http_policy_init](../frontend/common/http_policy.h#L31) инициализирует [FVM_DesktopHTTPPolicy](../frontend/common/network_loader.h#L13): без redirects,
connect10с/total30с, штатный trust. Явные native callbacks [fvm_desktop_socket_fetch_policy](../frontend/common/socket_http.h#L72)
и [fvm_desktop_socket_transfer_policy](../frontend/common/socket_http.h#L76) требуют policy в userdata; worker-вариант также
заимствует [FVM_NetControl](../ports/network/socket.h#L21). Старые socket options/controlled requests/callbacks сохранены.

max_redirects=0..FVM_HTTP_REDIRECT_MAX (8), connect_ms>0 и <=total_ms,
total_ms<=[FVM_NET_MAX_MS](../ports/network/socket.h#L8) (120с), необязательный ca_file. Неверная policy не запускает I/O.
Общий монотонный бюджет включает все hops/headers/body и прошедшее время DNS;
переполнение ticks учтено. DNS/CA/user callbacks принудительно не прерываются.
Job deadlines и отмена хоста по-прежнему действуют на всю цепочку.

Поддержаны GET301/302/303/307/308. Единственный непустой Location разрешается относительно
текущего URL строгим ASCII HTTP(S) resolver. Работают относительные пути, query и authority.
Credentials, controls, пробелы, неверные escapes и посторонние схемы отклоняются до
следующего соединения. HTTPS→HTTP запрещён. Другой origin разрешён в заданных пределах:
это транспортная политика хоста, не реализация Flash sandbox/domain authorization.

Промежуточные тела не передаются sink и не ожидаются; соединение закрывается после
ограниченных headers. Missing/duplicate/overlong Location, прочие3xx, self-reference
и исчерпание hop budget дают отказ. Циклы из нескольких URL ограничены тем же бюджетом;
постоянного redirect cache нет. При ошибке конечной передачи sink мог получить частичные
байты: их надо отбросить. Точный sink status сохраняется. Resolved не меняется до полного
успеха, затем содержит конечный canonical URL без fragment.

Native regression использует независимые raw HTTP ответы, binary NUL, все пять statuses,
relative/query/authority/other-host, ошибочные headers/URI, disabled/overflow limits,
loops, sink failure, timeout/cancel. Прежние URL/framing тесты проходят. Полная browser/
Flash сеть, foreign runtime/device acceptance не заявляются.

Нормативная ссылка: [RFC9110, Location и redirects](https://www.rfc-editor.org/rfc/rfc9110.html#section-10.2.2).

## Аналогичный libcurl-профиль и проверяемый TLS (v0.688)

[fvm_desktop_http_fetch_policy](../frontend/common/http.h#L43) и [fvm_desktop_http_transfer_policy](../frontend/common/http.h#L47) используют те же
policy/URI/chain loop, что socket-провайдер. Worker требует linked SDK с thread-safe
global APIs. libcurl сам redirects не выполняет: ограниченный header callback
останавливает передачу до промежуточного тела, следующий hop проверяется явно.
Этот профиль отключает environment proxies; authentication/cookies/decompression
не добавлены. Старые curl callbacks сохраняют прежнее поведение.

ca_file применяется к каждому TLS hop через verified trust API; null оставляет
штатные defaults провайдера. Проверка peer/hostname не отключается. Тест с локальным
временным CA проверяет HTTP→HTTPS, HTTPS→HTTPS, неверный сертификат на следующем
hop и запрет HTTPS→HTTP до обращения к plain-server. Оба провайдера проходят один
raw HTTP regression и TLS chain tests. Старые callbacks по-прежнему отвергают
redirects. DNS/CA/provider code могут задержать cleanup; hard preemption и полная
Flash/browser policy не заявляются.

## Player/IDE и консоль (v0.689)

[fvm_desktop_network_policy_init](../frontend/common/network_loader.h#L13) доступен в host SDK даже без провайдера.
[fvm_desktop_run_source_network](../frontend/common/network_loader.h#L22) и [fvm_desktop_run_network](../frontend/common/network_loader.h#L28) объединяют обязательные
HTTP policy и load monitor со штатным worker-провайдером. Layout старых options/
source/loader/monitor сохранён. Неверные policy/monitor отклоняются до I/O/callback/
window. Без провайдера local/memory работают, network input не поддержан.
Observer stop завершает весь playlist.

`--redirects N` принимает строгий integer0..8, требует --async-loads либо
--background-loads. Missing/duplicate/negative/fractional/out-of-range дают отказ до
запуска; после -- это буквальное имя файла. CLI выбирает тот же C policy runner,
сохраняя --load-timeout/--load-progress. Без opt-in redirects по-прежнему запрещены.
Конечный URL становится source base фильма, включая потомков memory-root и root
replacement. max_loads считает логические ресурсы; у HTTP hops отдельный лимит.

[Пояснённый C/server пример](../examples/player/redirect_loading/README.ru.md)
проверяет четыре ресурса/восемь запросов, перенесённые root/child/forms. Реальный
плеер проверен interpreter/JIT, waiting/async, native HTTPS, C source/memory,
stop-before-I/O, неверная policy и прежний no-redirect negative control.
Это ограниченные GET загрузки, не полные Flash network APIs.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_NET_MAX_MS` | macro / define | [ports/network/socket.h:8](../ports/network/socket.h#L8) |
| `FVM_NetControl` | struct | [ports/network/socket.h:21](../ports/network/socket.h#L21) |
<!-- source-links:end -->
