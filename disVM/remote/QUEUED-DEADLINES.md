# Queued proxy deadlines and sustained forwarding

Copyright (c) 2026 bhgv

EN: Preserve the original caller budget through broker queueing and reuse bounded
forwarding actor bookkeeping after completion. No global exclusive/stop is added.
RU: Сохранить исходный caller budget при ожидании broker queue и переиспользовать
bounded actor bookkeeping после completion. Глобального exclusive/stop нет.

## Просроченный вызов не получает новую жизнь

Раньше Proxynet.offer создавал свой alarm, но Request не содержал deadline.
Broker после ожидания в mailbox начинал новый wire timeout. Caller уже мог
получить Unknown, а ещё не отправленная Write позднее меняла original owner.
Семантика Unknown допускает неизвестный исход принятого запроса, однако не требует
исполнения заведомо просроченной работы, ещё не переданной владельцу.

Теперь Request хранит absolute monotonic deadline, вычисленный до allocation
reply/request/alarm channels. Broker проверяет его до reservation результата,
Retain, consume Drop и выделения wire ID. Expired queued operation получает
NotSent/empty image; connection остаётся usable. Даже expired Stop не закрывает
broker. Следующий actual wire request получает следующий последовательный ID.
Уже вернувшийся по alarm caller сохраняет свой Unknown: автоматического replay нет.

Wire exchange использует минимум original caller deadline и настроенного
connection exchange budget. Encoding, write и read разделяют один deadline;
очередь не получает новый бюджет. Automatic GC Drop ограничен исходным idle
budget broker, а не новым сроком после allocation. Этот local Request field не
попадает в DVP3 и не меняет public Dis64 kinds/opcodes/layout.

Если deadline истёк уже после broker admission/reservation/Drop consumption,
exchange отказывает с terminal NotSent либо Unknown и закрывает local connection
state. Частично отправленный frame всегда terminal Unknown. Broker не продолжает
поток с пропущенным wire ID или неоднозначным consumed ownership. Host по-прежнему
владеет OS FD; remote server имеет свой конечный cleanup deadline. Это не rollback
принятого remote effect и не hard real-time гарантия планировщика.

Такой же guard установлен в Proxyowner.serve перед Stop/Retain/Drop и перед
native begin/pins для ordinary/routed/export requests. До этого local queue
сохраняла deadline, но owner использовал его лишь внутри routed callback.
Отдельный actual original-owner gate держит callback до retirement queued Write
caller. Проверяются десять expired operation variants без изменения storage/refs;
private omission именно owner guard даёт actual Write=999 и штатный Stop owner.

## Ограничение одновременной работы вместо пожизненной квоты

Registry раньше увеличивал issued до 256 и никогда не снимал завершённые actors.
Даже после Drop/GC всех views соединение окончательно теряло возможность import.
Теперь array имеет 16 outstanding completion slots. Перед import registry
неблокирующим alt снимает только готовые completions, выполняет managed retirement
fence по exact direct-child PID и освобождает slot. Runtime отдельно отвечает за
reap/join native resident threads; общие counters по-прежнему обязаны сойтись.

Live actor никогда не вытесняется ради нового import. При заполнении array
возвращается Capacity до source Retain/native export. После завершения actors
следующий import восстанавливает доступную ёмкость. Close ждёт все оставшиеся
completion slots; completed channels не содержат Ref и не мешают его GC.
Native view handle generations, source ownership и counted peer Retain/Drop
сохраняют прежнюю семантику. Неиспользуемая registration template остаётся отдельным
owner до shutdown service.

## Проверки на реальных владельцах

Delayed-reply relay между двумя actual VMs удерживает первый Read до возврата
Unknown у второго queued Write caller. После release первый Read и следующий
Read видят original value 10. Просроченные Drop/Retain/result Import/fetch/Stop
не меняют leases, exports, IDs и usable connection. Private bytecode, намеренно
продлевающий Request deadline, даёт actual late Write=999; клиент обязан завершиться
с точной semantic error, а original server подтвердить значение 999. Crash,
compiler error, timeout или synthetic model этого доказательства не заменяют.

Отдельная architecture workload proxy-churn запускает три actual VMs: forty
import/Add/Drop cycles на одном connection повышают A с 10 до 50, B local остаётся 0.
Consumed C leases не оживают при reuse native slot/generation. Нужны сорок actual
B wrapper GC enqueue/drain и окончательные joins/root/export counters. C/JIT и
оба concurrent polling paths проходят одинаковую production библиотеку.
При private omission reaping после sixteen successful effects следующий import
даёт точно Capacity=4, A остаётся 26; оба server owners завершаются штатно.

## Диагностика CI и границы evidence

Предыдущая publication 3a44cd65 имеет TSan CI failure в routed callback fixture:
сбой произошёл при 20-second caller budget. TSan race report
в журнале отсутствовал. Точный внутренний status тогда не печатался; relay затем
маскировал исходный VM error сообщением truncated proxy record. Поэтому один этот
лог не доказывает конкретную runtime причину. Четыре локальных повторения исходной
матрицы под TSan на двух CPU прошли. Дефект renewed queue deadline установлен
отдельным детерминированным real-owner probe, не приписан CI по предположению.

Положительный overlap fixture теперь использует 60-second caller/connection и
30-second actor budget; source server — 45 seconds. Actual overlap gate двух
разных original function PIDs и expected 32 Adds остаются обязательными.
Намеренный missing-GC control сохраняет five-second idle fallback в отдельном
private fixture. Ошибки routed client теперь содержат status/value/round, а relay
сохраняет первичную VM error. Короткие deadline/fault/queue проверки не ослаблены.

Сроки не становятся синхронными между VM; transport authentication, dynamic
routing, general closures и distributed cycle tracing остаются отдельными задачами.


Whole-network omission suite получает 360 seconds общего CTest budget: внутри
много отдельных bounded real-VM cases, а прежний TSan прогон занимал 174 seconds
при лимите 180. Guest join, wire/caller deadlines и требования exact semantic
failure остаются прежними. Новый churn/queue/owner omission блок зарегистрирован
отдельно, чтобы не добавлять его время в этот aggregate suite.


RV64/QEMU в той же previous CI publication завершил 48/49: единственный отказ —
whole-network workload, остановленный ровно через 1200 seconds. Поэтому architecture
selection теперь разделяет coverage, сохраняя все C/JIT/poll combinations:

| Case | Что исполняется |
| --- | --- |
| network-proxy | Базовые images/results/ref actors, replay, GC и transport faults |
| forwarding-proxy | Три VM, opaque imported refs, orphan reply, oracle и оба hop loss |
| proxy-deadlines | Две реальные очереди с original caller expiry и инертными lifecycle requests |
| proxy-churn | Forty import/Add/Drop cycles и повторное использование actor bookkeeping |

Каждый workload сохраняет 1200-second process bound. Общий RV64/QEMU job budget
повышен с 45 до 75 minutes для расширенной матрицы; wire deadlines этим не меняются.
Отдельная ошибка/timeout каждого workload остаётся отказом всей architecture.
Это изменение группировки тестов, не заявление об успешном RV64 replay новой серии.


В concurrent success fixtures восемь queued callers имеют 60-second caller budget;
ordinary base wire budget остаётся five seconds. Трёх-VM success fixtures используют
60-second caller/source connection и 30-second B actor/service budget. Churn caller
получает 240 seconds на forty imports как целый процесс; timeout каждого import/Add/
Drop сохраняется. Это границы нагрузки, а не продление просроченного запроса в broker.
Новые queue отказные проверки по-прежнему используют one-second caller expiry.


Итог локальной проверки: полный обычный CTest 86/86 и повторный base network case
после последней настройки concurrent caller budget прошли. Под TSan закрыты все
87 cases: 79 прошли в полном запуске, остальные восемь повторно после завершённой
пересборки и запуска двух workloads одновременно. Повторный набор прошёл 8/8,
включая base network, forty-import churn, оба блока omissions и GC/root controls.
Отдельные queue/owner probes также прошли все четыре C/JIT/poll combinations
обычной и TSan сборок. Новая GitHub publication запускает собственную architecture
matrix; pending CI не является подтверждением ARM64/RV64 этой ревизии.


Проверки queued expiry дополнены [handoff deadlines](HANDOFF-DEADLINES.md):
исходный absolute deadline теперь передаётся через source actor, broker и actual
owner, включая startup Retain и typed result callbacks. Преобразование уже
заданного deadline в новый relative timeout на этих переходах устранено.
