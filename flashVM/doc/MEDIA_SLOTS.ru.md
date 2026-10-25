# Фиксированные слоты и части медиа

[English](MEDIA_SLOTS.md). Отдельный C89 API готовит двоичные S-записи для
файловых и потоковых каналов. Формат S-записи и стандартные сообщения 9P не меняются.

Мелкие команды, свойства, ответы и события продолжают использовать обычные
S-записи переменной длины без заполнения. Фиксированный размер не является
общим требованием канала. Профиль слотов включается явно только для крупных
объектов/медиа; небольшой объект можно целиком передать обычной S-записью.


## Профиль восстановления 1

До приёма данных стороны согласуют профиль через аутентифицированный надёжный
управляющий канал. Один слот занимает ровно 1024 байта: обычная S-запись
(checksum, length, локальный для направления tag, payload), затем нулевое
заполнение. Все целые сохраняют прежнее каноническое big-endian кодирование.
Запись tag 0 допустима, но библиотека сама не устанавливает привязки типов.

Пакет содержит от нуля до 64 полных слотов. При повреждении записи, в том числе
поля длины, следующий слот всё равно начинается через 1024 байта от известной
границы. Проверяются checksum и всё заполнение каждого слота. Неполный последний
слот отвергает весь пакет до callback: TCP-префикс нужно накопить, усечённую UDP
датаграмму — отбросить. encoded_size самой записи не включает заполнение;
успешный used слота всегда равен 1024. Для повреждённого слота callback получает
ошибку и NULL вместо записи.

Это работает при известном начале и размере блока. Поиск границы после вставки
или удаления байтов в произвольном потоке не реализован. Данные не исправляются;
коллизии checksum не исключены. Правила ширины checksum S-записи не меняются.
Не следует восстанавливать границу поиском случайно совпавшей CRC.

## Интерфейс C

Функции не выделяют память, не делают I/O и требуют последовательного доступа
владельца. View заимствует входные байты. Input/output/state не перекрываются.
Нет VM/SDL указателей, GC roots, скрытых потоков и глобального состояния.

| API | Владение, результат, ошибки |
| --- | --- |
| [`fvm_sslot_write`](../include/flashvm/sslot.h#L19) | Готовит запись и нули во временном буфере, затем публикует ровно 1024 байта. Выход принадлежит клиенту. Недостаточный объём LIMIT, неверные аргументы INVALID; при ошибке output/used не меняются. Payload может содержать любые двоичные данные и вложенные записи. |
| [`fvm_sslot_read`](../include/flashvm/sslot.h#L27) | Читает один слот: короткий ввод MORE, длинный INVALID. Ошибки формата/checksum/заполнения не меняют view/used. Внутренний MORE полного слота превращается в INVALID. Успешный view действует до изменения входа. |
| [`fvm_sslot_batch`](../include/flashvm/sslot.h#L50) | Обходит до 64 слотов независимо. Неверный размер INVALID, превышение квоты LIMIT до callback. OK пакета не означает исправность всех слотов: нужно проверить report/notifications. Callback не прерывает обход и не меняет ввод. Report публикуется после обхода. |

## Границы транспорта

Tversion/Rversion согласует msize полного сообщения 9P с заголовками. В данных
файла допустимы несколько S-записей и части большого объекта. Это содержимое
файла, а не дополнительные запросы 9P. Чтение может разделить слот; клиент его
накапливает. Существующий адаптер operation-файлов по-прежнему принимает одну
команду до [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9): новый модуль не добавляет туда скрытый медиа-endpoint.

По TCP можно объединять слоты в пределах согласованных ограничений. Для UDP
немедленного воспроизведения размер датаграммы должен учитывать проверенный
path MTU и темп отправки. Возможен один слот 1024 байта на датаграмму; большой
LAN-пакет требует отдельной политики. Пакет 64 КиБ не считается допустимым для
произвольного интернет-пути. Потеря и перестановка обрабатываются на уровне
объекта. UDP checksum может отбросить всю датаграмму ещё до приложения. CRC
S-записи не аутентифицирует отправителя. Управление, привязки тегов, запросы
повтора и изменение темпа передаются через надёжный аутентифицированный TCP.

Первоисточники: [9P version](https://9fans.github.io/plan9port/man/man9/version.html),
[RFC 8085](https://www.rfc-editor.org/info/rfc8085/).

## Сборка и проверка

sslot.c включён в flashvm_srecord и SDK. Для standalone SDL не требуется.
Общие инструкции: [сборка](BUILD_PLATFORMS.md).

```sh
cmake --build build --target flashvm_srecord_oracle
ctest --test-dir build -R flashvm_srecord_slot_oracle --output-on-failure
```

Независимый Python oracle с big integers/zlib проверяет точные байты, каждый
усечённый префикс, атомарность выхода, заполнение, повреждённую длину, 400
воспроизводимых изменений и настоящие TCP/UDP IPv4 loopback-сокеты. Сокеты
принадлежат тесту; сетевой listener плеера этим не добавляется.
## Части файлов и live v0.741

`smedia.h` задаёт opt-in тип `flashvm.media.fragment`. Ненулевой tag привязывается
отдельно для каждого направления через доверенный надёжный канал. Payload:
восемь канонических неотрицательных S-целых `version=1, mode, stream, epoch,
object, time_ms, total, part`, затем сырые байты. FILE=1, LIVE=2. Stream/epoch/object
ненулевые Uint32. Time_ms — относительное время представления Uint32; смысл и
часы задаёт хост, автоматического планировщика нет. После переподключения нужна
новая согласованная epoch; генерации identity и автоматического wrap нет.

Каждая нефинальная часть содержит ровно 768 байт, последняя — точный остаток.
Offset вычисляется как part*768; произвольных перекрывающихся offsets нет.
Total — 1..3145728 байт, до 4096 частей, bitmap 512 байт. Это предел одного
объекта, не всего файла/соединения. Большие файлы/видео используют несколько
объектов; порядок и итоговое завершение принадлежат хосту. Пустой файл передаётся
обычным файловым/управляющим протоколом, не нулевым bulk-объектом. Профиль слотов
выбирается явно, а не из недоверенных пакетов данных.

Receiver копирует заранее согласованную identity и заимствует буфер минимум
на total байт. Первый пакет не выбирает identity. Перед копированием проверяются
checksum, формат, tag и identity. Одинаковый дубликат идемпотентен; другие байты
у принятой части или другая identity дают CONFLICT. Чужой tag UNKNOWN, неверные
метаданные INVALID, короткий слот MORE, малая ёмкость LIMIT. Ошибки сохраняют
buffer/receiver/complete. Инициализированы только принятые части: пропуски содержат
старые произвольные байты, весь объект готов только при complete. Read выдаёт
заимствованный piece для немедленного потребителя. Итогового digest всего файла
и криптографической аутентификации этим не добавлено.

FILE missing перечисляет пропуски по возрастанию порциями. Хост передаёт запросы
через надёжный доверенный TCP с ограничением частоты и хранит исходные байты до
подтверждения завершения. Повтор сохраняет identity, номер и данные части.
LIVE missing всегда возвращает ноль: по deadline хост пропускает потери, выполняет
поддерживаемую кодеком маскировку и явно инициализирует следующий объект. Части
текущего объекта можно принимать вне порядка; старые object/epoch отвергаются.
Автоматического вытеснения/deadline нет. Это сборка/политика повторов, не декодер,
A/V синхронизация, STUN, pacing, UDP authentication, listener или воспроизведение
сетевого медиа плеером.

| API | Контракт |
| --- | --- |
| [`fvm_smedia_write`](../include/flashvm/smedia.h#L72) | Точная часть в один слот 1024 байта, без выделений; output/used атомарны при ошибке. |
| [`fvm_smedia_read`](../include/flashvm/smedia.h#L79) | Проверяет слот, согласованный tag и точный размер; заимствованный piece только при OK. |
| [`fvm_smedia_receiver_init`](../include/flashvm/smedia.h#L86) | Копирует доверенную identity, очищает bitmap, заимствует достаточный буфер; байты буфера не меняет. Ошибка сохраняет старый receiver. |
| [`fvm_smedia_accept`](../include/flashvm/smedia.h#L96) | Проверяет и копирует новую часть либо принимает идентичный дубликат. Complete означает получение всех частей; ошибки не меняют выходы. |
| [`fvm_smedia_missing`](../include/flashvm/smedia.h#L105) | Выдаёт порцию FILE-пропусков от start, продолжение last+1; LIVE — ноль. Без I/O, таймеров и отправки запросов. Count меняется только при OK. |

[Исполняемый пример](../examples/embedding/media_parts/README.ru.md) показывает
повреждённую середину, выборочный повтор файла, пропуски live и компактное
управление. Независимый Python oracle проверяет байты, префиксы, метаданные,
последний бит bitmap, переставленные UDP-части и компактный TCP-запрос повтора.
Сокеты — часть теста; native-модули не зависят от транспорта.
## Согласование размеров через tag 0 v0.742

Tag 0 многофункционален. Первый integer payload 1 сохраняет старую схему
неизменяемых привязок типов. Integer 2 выбирает новую служебную оболочку:
ровно восемь канонических неотрицательных S-целых
`2, command, id, tag, record_limit, slot_bytes, part_bytes, reserved=0`.
Command 1 — OFFER, 2 — ACCEPT. Неизвестная версия службы/команда даёт UNKNOWN
без изменения выходов. Хост явно маршрутизирует известные службы; неизвестное
не передаётся в registry привязок и не считается правом доступа. Другие
служебные команды можно добавить в дальнейшем.

Tag — уже привязанный тип одного направления. Record_limit ограничивает
закодированные tag+payload, без внешних checksum/length и заголовка 9P.
Slot_bytes=0 и part_bytes=0 выбирают обычные компактные записи. Bulk-слот —
256..65536 байт; record_limit <= slot_bytes-32, record_limit >64,
1 <= part_bytes <= record_limit-64. Запасы оставляют место заголовкам.
Выбранный объём должен представляться native size_t. Ceiling — реальные
буферы/квоты хоста; сама библиотека память не выделяет. Fixed-capable ceiling
допускает компактный режим. Направления могут согласовать разные tag/type/size
и используют раздельные exchange.

Init не активирует формат. Propose создаёт pending OFFER. Respond выбирает
пределы не больше обоих предложений и сохраняет pending ACCEPT. Получатель
активирует свой точный выбор после успешной постановки ACK в надёжную доверенную
очередь; инициатор — после проверки коррелированного ограниченного ACK.
Точный повтор уже committed ACK идемпотентен. Чужой tag/ID, увеличенный ACK и
параллельное согласование дают CONFLICT, неверные поля INVALID. Ошибки сохраняют
active. ID возрастают без повторов до Uint32; затем нужна новая сессия.
Cancel оставляет active и не разрешает повторное использование ID. Поздний ACK
отменённого запроса не активируется. Неопределённое состояние после разрыва
управления требует сверки либо новой сессии до возобновления данных. Это не
распределённый консенсус и не криптографическая аутентификация.

Перед сменой формата хост останавливает старые передачи. Active-ограничения
применяются к соответствующему файловому/медиа-потоку: generic slot reader
проверяет границы, а не произвольную per-tag квоту. Для обычных payload нужно
применять agreed record_limit в [fvm_srecord_read](../include/flashvm/srecord.h#L94) до dispatch. Msize 9P не меняет
автоматически квоту S-записи или размер UDP-датаграммы. Сами служебные записи
остаются короткими, переменной длины.

| API | Выполнение и владение |
| --- | --- |
| [`fvm_scontrol_write`](../include/flashvm/scontrol.h#L75) | Компактная проверяемая служебная запись; output/used клиента атомарны при ошибке. |
| [`fvm_scontrol_read`](../include/flashvm/scontrol.h#L79) | Проверяет одну запись, копирует метаданные; последующие записи не потребляются. UNKNOWN не меняет выходы. |
| [`fvm_scontrol_init`](../include/flashvm/scontrol.h#L86) | Копирует реальные локальные пределы, очищает pending/active, без выделений. |
| [`fvm_scontrol_propose`](../include/flashvm/scontrol.h#L92) | Один pending OFFER с возрастающим ID в локальных квотах; active/transport не меняет. |
| [`fvm_scontrol_respond`](../include/flashvm/scontrol.h#L99) | Выбирает меньшие квоты, сохраняет pending ACK; выход только при успехе. |
| [`fvm_scontrol_commit`](../include/flashvm/scontrol.h#L108) | Проверяет корреляцию и ограниченный/точный ACK, атомарно меняет active; надёжная очередь и quiescence принадлежат хосту. |
| [`fvm_scontrol_cancel`](../include/flashvm/scontrol.h#L115) | Удаляет pending, сохраняет active/историю ID; удалённое состояние не откатывает. |
| [`fvm_sslot_write_sized`](../include/flashvm/sslot.h#L57) | Проверяемая padded-запись согласованного размера без scratch размером со слот; output/used атомарны. |
| [`fvm_sslot_read_sized`](../include/flashvm/sslot.h#L62) | Проверяет полный согласованный слот и заполнение; выдаёт borrowed record только при OK. |
| [`fvm_sslot_batch_sized`](../include/flashvm/sslot.h#L66) | До 64 слотов согласованного размера; следующая граница не зависит от повреждённого содержимого. |
| [`fvm_srecord_write_pair`](../include/flashvm/srecord.h#L84) | Два заимствованных диапазона как одна запись, без большого промежуточного буфера; байты/CRC равны конкатенации. Ошибки атомарны. |
| [`fvm_smedia_write_profile`](../include/flashvm/smedia.h#L112) | Согласованные slot/part, прежняя схема fragment и bounded identity; scatter header+data. |
| [`fvm_smedia_read_profile`](../include/flashvm/smedia.h#L118) | Проверяет типизированную часть с согласованной геометрией, выдаёт borrowed bytes; пакет не меняет profile. |
| [`fvm_smedia_receiver_init_profile`](../include/flashvm/smedia.h#L122) | Копирует согласованные profile/identity, заимствует достаточный буфер. Старые entry points сохраняют opt-in профиль 1024/768. |

Profile копируется в receiver. Для его смены нужны quiescence и явно
инициализированный новый объект. Total остаётся до 3 МиБ, частей до 4096 даже
при больших слотах. Count/offset/остаток вычисляются по agreed part_bytes.
В этой серии native receiver получил поле profile: заголовки и библиотека должны
быть от одной сборки; native layout нельзя сериализовать.

Пример согласует размеры через компактный tag 0 до передачи bulk. Независимый
oracle уменьшает 65536 до 4096/4000 и собирает настоящий объект с этими частями,
проверяет compact mode, отменённые/старые ACK, атомарность и слоты до 64 КиБ.
Native C89 contract проверяет этот же обмен для sanitizer/cross-link.
## Смена размеров во время live-передачи

OFFER_AT=3 и ACCEPT_AT=4 дописывают после восьми общих полей три канонических
Uint32: `stream, epoch, first_object`. Все ненулевые; ACK повторяет точную
границу. Команды 1/2 сохраняют восемь полей для generic-канала с остановленными
хостом старыми передачами. [`fvm_scontrol_propose_at`](../include/flashvm/scontrol.h#L123) позволяет уменьшить или
увеличить профиль в пределах прежних ceiling обеих сторон. Новый возрастающий
request ID нужен даже для возврата к ранее использованному размеру.

Новый профиль применяется только к последующим объектам. Старый объект нужно
завершить либо отбросить со старым receiver; частичный буфер нельзя заново
толковать с другой длиной части. Хост выбирает будущий object по своему счётчику
сегментов, без остановки всего таймлайна. [`fvm_smedia_receiver_init_agreed`](../include/flashvm/smedia.h#L132)
копирует committed geometry и отвергает чужой stream/epoch либо object раньше
first_object. Поздние UDP-пакеты старого профиля не проходят проверку размера
или ожидаемой identity и не меняют новый буфер. При временном сосуществовании
профилей хост хранит отдельно ограниченные old/new receivers и маршрутизирует
только доверенные известные identity. Библиотека не создаёт скрытых receivers
и не опустошает сокеты.

При высоком проценте потерь можно запросить меньший слот, после устойчивого
периода низких потерь/задержки — больший, в пределах path MTU и ресурсов.
Автоматики здесь нет: причина потерь может быть другой. Политика хоста должна
использовать окна наблюдения, hysteresis/cooldown и pacing, а не менять размер
на каждом пропуске. Оценщик потерь, latency probe и congestion controller этим
не добавлены. Управление/ACK — надёжный доверенный TCP, live-данные могут идти
по UDP. Прикладной слот 64 КиБ не означает датаграмму UDP такого размера.

| API | Контракт |
| --- | --- |
| [`fvm_scontrol_propose_at`](../include/flashvm/scontrol.h#L123) | Pending OFFER с возрастающим ID и точной будущей границей; active не меняется, ACK не может сдвинуть границу. |
| [`fvm_smedia_receiver_init_agreed`](../include/flashvm/smedia.h#L132) | Заимствует буфер при committed bulk-limits, копирует profile; чужой stream/epoch/старый object даёт CONFLICT до изменения состояния. |

Независимый oracle и C-contract выполняют 4096/4000 -> 512/400 -> 4096/4000
на последовательных live-объектах, отвергают сдвиг границы ACK и старые объекты,
собирают каждый объект без смешения размеров. Oracle добавляет поздние пакеты
предыдущего профиля. Это контракты передачи/формата, не автоматическая адаптация
или сетевое воспроизведение desktop-плеером.
## Проверка v0.740–v0.742

Прошли 53 выбранных native-контракта: 52 в первом прогоне и старый heartbeat
пример после восстановления потерянного хостом executable-бита. Прошли все 13
shared-host проверок. Новый C89-contract и прокомментированный пример прошли
AddressSanitizer/UBSan; перечисление утечек отключено ограничениями хоста.
Инструментированы четыре протокольных модуля, а не вся VM. Независимые Python
oracles используют собственные integer/zlib wire bytes и реальные IPv4 loopback
TCP/UDP-сокеты.

C89-contract и пример слинкованы для ARM64 Linux, RISC-V64 Linux и Windows x64;
клиент публичных заголовков скомпилирован как C++98 для всех трёх. Иностранные
бинарники не запускались. Приёмка настоящих окон/аудиоустройств остаётся открытой.
Standalone cross-link из корня репозитория:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors -IflashVM/include \
  flashVM/tests/protocol/scontrol_contract.c flashVM/src/protocol/srecord.c \
  flashVM/src/protocol/sslot.c flashVM/src/protocol/smedia.c \
  flashVM/src/protocol/scontrol.c -o media-contract-arm64
```

Для других проверенных целей замените target на riscv64-linux-musl либо
x86_64-windows-gnu. Полный host требует target static SDL SDK и адаптер платформы:
[платформы](BUILD_PLATFORMS.ru.md), [desktop host](DESKTOP_FRONTEND.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `fvm_scontrol_cancel` | function | [include/flashvm/scontrol.h:115](../include/flashvm/scontrol.h#L115) |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:108](../include/flashvm/scontrol.h#L108) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:86](../include/flashvm/scontrol.h#L86) |
| `fvm_scontrol_propose` | function | [include/flashvm/scontrol.h:92](../include/flashvm/scontrol.h#L92) |
| `fvm_scontrol_propose_at` | function | [include/flashvm/scontrol.h:123](../include/flashvm/scontrol.h#L123) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:79](../include/flashvm/scontrol.h#L79) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:99](../include/flashvm/scontrol.h#L99) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:75](../include/flashvm/scontrol.h#L75) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:96](../include/flashvm/smedia.h#L96) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:105](../include/flashvm/smedia.h#L105) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:79](../include/flashvm/smedia.h#L79) |
| `fvm_smedia_read_profile` | function | [include/flashvm/smedia.h:118](../include/flashvm/smedia.h#L118) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:86](../include/flashvm/smedia.h#L86) |
| `fvm_smedia_receiver_init_agreed` | function | [include/flashvm/smedia.h:132](../include/flashvm/smedia.h#L132) |
| `fvm_smedia_receiver_init_profile` | function | [include/flashvm/smedia.h:122](../include/flashvm/smedia.h#L122) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:72](../include/flashvm/smedia.h#L72) |
| `fvm_smedia_write_profile` | function | [include/flashvm/smedia.h:112](../include/flashvm/smedia.h#L112) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:94](../include/flashvm/srecord.h#L94) |
| `fvm_srecord_write_pair` | function | [include/flashvm/srecord.h:84](../include/flashvm/srecord.h#L84) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:50](../include/flashvm/sslot.h#L50) |
| `fvm_sslot_batch_sized` | function | [include/flashvm/sslot.h:66](../include/flashvm/sslot.h#L66) |
| `fvm_sslot_read` | function | [include/flashvm/sslot.h:27](../include/flashvm/sslot.h#L27) |
| `fvm_sslot_read_sized` | function | [include/flashvm/sslot.h:62](../include/flashvm/sslot.h#L62) |
| `fvm_sslot_write` | function | [include/flashvm/sslot.h:19](../include/flashvm/sslot.h#L19) |
| `fvm_sslot_write_sized` | function | [include/flashvm/sslot.h:57](../include/flashvm/sslot.h#L57) |
<!-- source-links:end -->
