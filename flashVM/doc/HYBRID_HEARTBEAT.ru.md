# Гибридная политика UDP/TCP

Модуль C89 `rheartbeat.h` позволяет независимо выбрать частую проверку UDP,
редкую проверку TCP и внеочередную проверку TCP по аутентифицированному UDP-
уведомлению с ограничением частоты. Реализованы политика и компактный кодек;
STUN-сервер, сокетный адаптер и аутентификация транспорта не добавляются.
[Исполняемый пример политики](../examples/embedding/hybrid_heartbeat/README.ru.md),
[наблюдатели](RESOURCE_EVENTS.ru.md), [отладчик](RESOURCE_DEBUGGER.ru.md).

## Стоимость и групповая доставка

Схема полезна, но heartbeat уже установленного TCP не повторяет handshake.
Меньший заголовок UDP сам по себе не доказывает меньшие затраты: существенны
частота, ACK, аутентификация, NAT и пробуждения радио/CPU. RFC 8085 §3.5 отдельно
описывает случаи, когда более редкие проверки TCP экономнее UDP.
[RFC 8085](https://www.rfc-editor.org/rfc/rfc8085.html#section-3.5).

LAN broadcast/multicast подходит как адаптер обнаружения и групповых уведомлений.
Через интернет/NAT универсальная доставка так не получается: нужен fan-out/relay
с аутентификацией и индивидуальными правами. Два наблюдателя отладчика читают
независимые очереди, а команды изменения сериализует владелец. Это не выбор
распределённого лидера. Общая UDP-рассылка требует ограничения суммарной скорости
и обратной связи. [RFC 8085 §4](https://www.rfc-editor.org/rfc/rfc8085.html#section-4).

UDP не сохраняет отдельное TCP-отображение NAT. Проверка TCP должна пройти по
самому TCP-соединению; STUN сообщает адрес наблюдаемого транспорта/соединения.
UDP может запросить эту проверку либо создание нового TCP к настроенному peer.
[RFC 8489 §6.3.1.1](https://www.rfc-editor.org/rfc/rfc8489.html#section-6.3.1.1).
Прикладная политика не заменяет и не настраивает OS TCP SO_KEEPALIVE.
[RFC 9293 §3.8.4](https://www.rfc-editor.org/rfc/rfc9293.html#section-3.8.4).

## API и владение

Состояние и настройки принадлежат вызывающему коду; VM-указателей нет. Все поля
policy и точный struct_size нужно заполнить. Один state изменяет один владелец
либо внешняя синхронизация. Внутренних locks, памяти, часов ОС, сокетов и VM-
вызовов нет. Ошибки не меняют state/outputs. Модуль входит и в flashvm_srecord.

| Функция | Аргументы, результат и ограничения |
| --- | --- |
| [`fvm_rheartbeat_init`](../include/flashvm/rheartbeat.h#L76) | State, копируемая policy, ненулевая Uint32 эпоха аутентифицированного соединения, monotonic Uint32 now_ms. Проверяет ABI, маски и пределы интервалов. Hints требуют UDP+TCP и положительного request_gap_ms. Нулевой timer отключает период/дедлайн; универсальных NAT-defaults нет. Повторный init сбрасывает replay: при reconnect нужна новая аутентифицированная эпоха. |
| [`fvm_rheartbeat_due`](../include/flashvm/rheartbeat.h#L81) | State, время, output-биты действий. Предлагает SEND_UDP/SEND_TCP, раздельные UDP_STALE/TCP_STALE и RECONNECT. Это не подтверждение отправки. Повторный poll может снова предложить неотправленное. STALE — подозрение, а не доказательство смерти peer и не отмена команд. |
| [`fvm_rheartbeat_activity`](../include/flashvm/rheartbeat.h#L86) | State, ровно UDP либо TCP, received 0/1, now_ms. Только после настоящей успешной операции. Send сбрасывает idle; receive также silence своего пути. UDP не обновляет TCP. TCP обслуживает запрос probe, но решение reconnect остаётся host. |
| [`fvm_rheartbeat_accept`](../include/flashvm/rheartbeat.h#L92) | State, копия hint и now_ms после проверки аутентификации/прав транспорта. Чужая эпоха/нет прав: DENIED; повтор/переупорядоченный sequence: CONFLICT; неверные поля/часы: INVALID. При OK отмечает получение UDP и объединяет hints. Команд VM и адресов назначения не принимает. |
| [`fvm_rheartbeat_clear`](../include/flashvm/rheartbeat.h#L97) | State, непустая маска hints и now_ms. Host подтверждает/отклоняет совет и заново отсчитывает throttle. Счётчики фактической активности/приёма не сбрасываются. Сетевых действий нет. |
| [`fvm_rheartbeat_next`](../include/flashvm/rheartbeat.h#L102) | State, outgoing flags, output-hint. Выдаёт возрастающий Uint32 sequence; исчерпание: LIMIT без изменений. Для I/O retry сохраните выданный hint. Локальные права входа не выдают прав на другом peer. |
| [`fvm_rheartbeat_hint_write`](../include/flashvm/rheartbeat.h#L106) | Tag, hint, output/capacity, used. Поля `version,epoch,sequence,flags` — канонические неотрицательные S-integer в проверенной записи. Неверные поля: S_INVALID, ёмкость: S_LIMIT. Ошибки не меняют output/used. MAC не добавляется. |
| [`fvm_rheartbeat_hint_read`](../include/flashvm/rheartbeat.h#L111) | Входные bindings, буфер/размер, output-hint, consumed. Нужна схема flashvm.transport.hint/version 1. Проверяет CRC и ровно четыре целых. Префикс: S_MORE, неизвестный тег: S_UNKNOWN, неверные поля/схема: S_INVALID; framing-статусы сохраняются. Outputs только при S_OK. Decode не разрешает accept автоматически. |

[`FVM_RHeartbeatPolicy`](../include/flashvm/rheartbeat.h#L28), [`FVM_RHeartbeat`](../include/flashvm/rheartbeat.h#L57), [`FVM_RHeartbeatHint`](../include/flashvm/rheartbeat.h#L47) — native-записи,
не wire layout. Таймеры: миллисекунды 0..[FVM_RH_MAX_INTERVAL](../include/flashvm/rheartbeat.h#L18). Часы Uint32 могут
оборачиваться. Откат или неоднозначный промежуток свыше 2^31−1 мс: INVALID.
Опрос нужен чаще примерно 24,8 суток. Возрастающие ages насыщаются, поэтому
долгое молчание не становится «свежим» после оборачивания часов.

Обычный трафик переносит idle-проверки. Только приём подтверждает получение с
соответствующего пути. Запрошенный TCP probe ждёт request_gap_ms после прошлого
обслуженного совета/TCP-активности и достаточного TCP idle. Reconnect ограничен
тем же throttle; его можно отклонить. При нулевой allowed_hints вход запрещён.

## Граница реализации

До accept нужна аутентификация unicast ingress: CRC, эпоха и sequence не являются
учётными данными. Ключи/MAC или защищённый транспорт, согласование эпохи, шифрование,
jitter, общий rate/congestion control и сообщение наблюдаемого IP/порта относятся
к транспортному адаптеру. Адреса и команды отправителя здесь не исполняются.
Совместимость wire со STUN не заявляется.

Host опрашивает политику из event loop, применяет свои квоты, сообщает успешный
I/O и сохраняет независимость команд/событий от потерь heartbeat. Неуспешный send
не вызывает activity. После обрыва нужно отвергать старую эпоху и явно отзывать
debugger/resource-сессии. Timeout не отменяет уже выполненные эффекты. Интервалы
30/300 секунд в примере — иллюстрация, а не измеренная гарантия NAT/энергозатрат.
Настоящий UDP/TCP listener, broadcast/multicast и internet traversal пока не добавлены.

## Проверки

Команды: [EN](HYBRID_HEARTBEAT.md#build-and-verification). Контракт проверяет
редкий TCP и ранний запрос через UDP, раздельные признаки приёма, throttle,
replay/epoch, ошибки без изменений, исчерпание sequence, wrap и насыщение ages.
Независимый Python oracle проверяет байты и границы целых, CRC и неверные префиксы.
Это тесты политики/кодека без ожиданий и сетевой приёмки. Cross-build не означает
исполнение на целевой платформе.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_RH_MAX_INTERVAL` | macro / define | [include/flashvm/rheartbeat.h:18](../include/flashvm/rheartbeat.h#L18) |
| `FVM_RHeartbeat` | struct | [include/flashvm/rheartbeat.h:57](../include/flashvm/rheartbeat.h#L57) |
| `fvm_rheartbeat_accept` | function | [include/flashvm/rheartbeat.h:92](../include/flashvm/rheartbeat.h#L92) |
| `fvm_rheartbeat_activity` | function | [include/flashvm/rheartbeat.h:86](../include/flashvm/rheartbeat.h#L86) |
| `fvm_rheartbeat_clear` | function | [include/flashvm/rheartbeat.h:97](../include/flashvm/rheartbeat.h#L97) |
| `fvm_rheartbeat_due` | function | [include/flashvm/rheartbeat.h:81](../include/flashvm/rheartbeat.h#L81) |
| `fvm_rheartbeat_hint_read` | function | [include/flashvm/rheartbeat.h:111](../include/flashvm/rheartbeat.h#L111) |
| `fvm_rheartbeat_hint_write` | function | [include/flashvm/rheartbeat.h:106](../include/flashvm/rheartbeat.h#L106) |
| `fvm_rheartbeat_init` | function | [include/flashvm/rheartbeat.h:76](../include/flashvm/rheartbeat.h#L76) |
| `fvm_rheartbeat_next` | function | [include/flashvm/rheartbeat.h:102](../include/flashvm/rheartbeat.h#L102) |
| `FVM_RHeartbeatHint` | struct | [include/flashvm/rheartbeat.h:47](../include/flashvm/rheartbeat.h#L47) |
| `FVM_RHeartbeatPolicy` | struct | [include/flashvm/rheartbeat.h:28](../include/flashvm/rheartbeat.h#L28) |
<!-- source-links:end -->
