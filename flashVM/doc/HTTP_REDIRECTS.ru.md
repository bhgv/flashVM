# Ограниченные HTTP GET перенаправления (v0.687)

[English](HTTP_REDIRECTS.md) · [Socket-транспорт](SOCKET_TRANSPORT.ru.md) ·
[Наблюдение загрузок](LOAD_MONITOR.ru.md).

[fvm_desktop_http_policy_init](../frontend/common/http_policy.h#L31) инициализирует [FVM_DesktopHTTPPolicy](../frontend/common/http_policy.h#L21): без redirects,
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
