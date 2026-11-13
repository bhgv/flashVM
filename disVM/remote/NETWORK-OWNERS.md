# Trusted-stream typed network owners

Copyright (c) 2026 bhgv

Две **настоящие DisVM** теперь обмениваются typed variable/function calls через
connected byte stream. В приёмке это два OS-процесса, настоящий loopback TCP,
восемь одновременных клиентов, C/JIT и оба register-poll paths. Код библиотек —
Limbo; функция исполняется исходным постоянным Prog, не C-моделью callback.

## Подключение и границы доверия

Порядок declarations: `sys.m`, `proxyhost.m`, `proxyhandler.m`, `proxyowner.m`,
затем `proxynet.m` для клиента либо `proxyserver.m` для сервера.
Собрать matching NC64 producer библиотеки `proxyowner`, `proxyframe`,
`proxystream`, `proxynet`, `proxyserver`: обычная native CMake-сборка уже включает
эти `.dis` в build. `init()` предшествует sharing/spawn.

`Proxyserver.serve(input, output, high, low, timeout, handler, cell, capture)`
создаёт отдельную session и исходного owner для пары exports. Клиентский
`Proxynet.start(input, output, high, low, timeout, ready)` запускается собственным
spawn. `ready` получает `(status, endpoint, variableImage, functionImage)`;
при отказе greeting status=Disconnected, endpoint=nil, images пусты.
`Proxynet.call` использует операции `Proxyowner.Read/Write/Invoke/Drop/Stop`.
Image содержит connection epoch и исходный owner handle. Проверяется target и
каждый использованный ref. Повторные refs сохраняют actual alias identity.

Направления FD исключительно принадлежат этому connection broker; caller
сохраняет FD живыми до completion/retirement. Проверенный socket путь — inherited
stdin/stdout, установленные trusted launcher. Публичного dial/listen здесь нет.
Новый Sys tuple gate Inferno не обходится и vendor bytes не меняются.

`high/low` — **128 бит уникального connection incarnation, предоставленные
launcher извне**. Их нельзя переиспользовать при reconnect/restart, даже если
новая VM повторила process-local session/slot/generation. В тестах константы
являются тестовыми данными; они не генератор production identities. Эпоха не
секрет и не authentication. Соединение должно быть доверенным/аутентифицированным
внешним transport. Открытый TCP без TLS/проверки peer этим API не становится
безопасным. Результаты не заявляют готовность untrusted public networking.

## Ограниченное состояние и исходы

Один connection broker последовательно передаёт запросы из bounded mailbox
capacity 16. Другие Prog и connections исполняются независимо. Исходный owner
держит actual Cell/capture/module roots; peer получает только handles и скаляры.
Внутри active callback сохраняется отказ от blocking nested proxy call.
Retirement public caller не означает, что приватные IO/session children уже
joined: они удерживают FD/roots до своего completion/deadline. Направления нельзя
передавать новому broker до завершения этой частной session.

Сервер допускает только следующий положительный ID. Хранится **одна** пара
точных request bytes / completed response bytes. Точный last ID возвращает
cached outcome, включая callback error или Drop, без второго owner effect.
Тот же ID с другим payload даёт Conflict=6. Старый ID после продвижения окна
получает Stale=5; старый эффект больше не допускается. Переполнение ID закрывает
broker, не оборачивает счётчик. Это ограниченный reply cache без отдельного ACK;
после разрыва cache не переносится в новую session и automatic replay отсутствует.

Успешный reply означает completed owner effect. Callback exception возвращает
OwnerError=-1 и допускает частичный effect. Caller `NotSent=-2` относится к
локальному offer; `Unknown=-3` после offer не доказывает remote admission либо
отмену. Ошибка/повреждение/потеря wire reply после попытки передачи даёт Unknown
и навсегда закрывает этот broker. Следующие calls получают Disconnected=-4 до
нового wire IO. Broker публикует terminal state и завершается; reconnect создаёт
новый endpoint/images с новым epoch.

Caller timeout включает local offer/reply. Он не передаётся как синхронизированное
время другой машине: уже переданный запрос может выполнить effect после Unknown.
Blind retry запрещён. Wire exchange использует один local absolute monotonic
deadline для передачи и ответа; сервер делит один deadline между read, owner call
и response. Медленная передача одного байта не продлевает frame budget.
`timeout` также ограничивает idle lifetime client broker; истёкший endpoint не
возобновляется. Очистка/join зависит от OS scheduling и не является hard real-time.

Private I/O supervisor владеет своими native IO/alarm children до join. Если
public transfer caller уничтожен, buffered completion позволяет supervisor
дожить до deadline и завершить pending IO. Отдельная private server session
аналогично сохраняет cleanup при уничтожении public serve caller. При EOF,
framing failure, timeout или Stop session делает killwait original owner;
его native retirement hook завершает active pins и передаёт exports Fast.

`Proxystream.transfer` возвращает status и известный progress. При timeout progress
равен -1: syscall мог уже commit bytes до kill boundary. Buffer нельзя менять
до завершения supervisor. Такой stream закрывается логически, partial frame не
продолжается на другом соединении. Таймаут не превращается в доказанную отмену.

Native stdin/stdout socket readiness теперь зарегистрирована в cancellation
ledger. `poll` не держит queue mutex; `recv/send` используют `MSG_DONTWAIT`, send
также `MSG_NOSIGNAL`. FD flags не меняются. EOF/error/partial output восстанавливают
исходный heap owner и отпускают staging/root pins. Статистика native GC и banner
completion выведены в stderr, чтобы stdout мог нести только application bytes.

## Приватный формат версии 1

Все числа — явно восемь big-endian bytes; signed scalar — битовый 64-bit pattern.
Record имеет ровно 152 bytes, magic `DVP1`, version 1. Managed pointers, C padding
и bitmap/layout native frame в stream не попадают. Это private hosted NC64
протокол, **не DIS64 format, public kind/opcode или Word=4/ref=8 ABI**.

| Byte offset | Содержание |
| --- | --- |
| 0..7 | Magic 4 bytes, version, type (Hello=1/Call=2/Reply=3), reserved zero 2 bytes |
| 8 | Exact record size 152 |
| 16, 24 | Connection epoch high/low |
| 32 | ID; Hello=0, request/reply positive, максимум signed 63 bits |
| 40, 48, 56 | Operation (Read=1..Stop=5), ref count 0..2, copied scalar value |
| 64, 88, 112 | Target/left/right: session, slot, generation по 8 bytes |
| 136, 144 | Status/result; в request обязаны быть нулевыми |

Hello передаёт variable в target и function в left. Slot ограничен 0..255,
owner session/generation в первом codec — неотрицательные signed 63-bit values;
реальные exports требуют ненулевых identities/generations. Reply должен точно
совпасть с request до byte 136, кроме type. Неизвестные status, epoch/version,
size/reserved, неверные ID/refs/slot и неполный кадр не становятся managed call.

## Приёмка и оставшаяся работа

`runtime/test_network_proxy.py` проверяет два реальных TCP VM, alias updates,
исключение после partial effect, explicit Drop/Stop, replay после потерянного
response, conflict, stale window и новый epoch при повторившихся native handles.
Проверяются fragmented requests, truncated/malformed header/payload, повреждение
correlation/epoch/status ответа, отсутствие ответа, bad greeting без capability,
trickle deadline, обрыв после **actual callback entry**, native output backpressure,
неизменность FD flags, retirement public IO/serve callers и idle images.
Shared native scalar results дополнительно проверяются четырьмя writers и
branch readers: [builtin publication contract](../runtime/BUILTIN-SCALARS.md).
Network test обнаружил raw Processctl.pid return store; исправленная публикация
не повторяет native side effect после BUSY. TSan control возвращает raw store и
обязан показать actual race.
Каждая positive VM проверяет matched native resident joins, отсутствие миграции,
concurrent GC policy, пустые RC queues, exports=0/active=0 и native staging=0.
Private compiled omissions epoch guard / duplicate cache обязаны дать конкретный
semantic failure; crash/compile failure/timeout не засчитываются.

Пока доступны int Cell, copied scalar result, один function/capture и максимум
два same-owner variable refs. Нет arbitrary closure/value types, String snapshots,
proxy-valued returns, cross-owner ref routing, transparent compiler lowering или
автоматического Retain/finalizer каждой копии Image. Drop отзывает общий export;
это ещё не distributed GC. TLS/peer authentication, automatic unique epoch
provisioning, owning dial/listen/FD adoption и полноценная cross-node lease/ACK
политика остаются отдельными границами. ARM64/RV64 выполняют ту же CI-приёмку;
QEMU не доказывает physical memory ordering или performance.
