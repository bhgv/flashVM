# Текущий статус многопоточности DisVM/Limbo

Copyright (c) 2026 bhgv

## 2026-11-15: внутренние native provider handles

Runtime0.122 хранит общий owning handle в ordinary descriptor slots. File и
Directory adapters реализуют immutable native operation tables. Read/write,
formatted output, seek, fstat и dirread вызывают provider dispatch; close-worker
получает owning handle через move и закрывает ровно один раз. Поиск handle идёт
до native wait, через прежние rooted FD/FIFO и GC admission. Нового global stop,
managed provider callback, allocation при передаче владения либо guest ABI нет.

[Контракт RU/EN](PROVIDER-HANDLES.md) явно оставляет specialized loader/stdio,
directory-abort recovery и kind admission в прежних adapters. Guest registration,
mount/walk, socket/channel/device providers и credentials ещё не подключены.
Native host rwx и прежние namespace/security limits сохраняются.

Provider unit проверяет независимый callback, unsupported outputs, refused moves,
очистку source и exactly-once close. Host-unit прошёл 30/30 в dynamic и static;
расширенный guest rwx прошёл оба execution/poll static paths. Close-backlog probe
перенесён со старого submit на новую consuming resource boundary: все 1536 guests,
64 pending jobs, files/directories FIFO, три GC profiles и полный drain прошли
в dynamic/static. Первый wide дал 69/70 из-за старой границы probe; финальный
wide после исправления прошёл 70/70. Handle dispatch также компилируется как
C89. Новая архитектурная CI учитывается отдельно. Предыдущая CI v0.1699
ещё выполнялась при начале этого этапа и не подтверждает новый provider layer.

## 2026-11-15: existing write-only open и закрытая прежняя RV64 приёмка

[CI 9d9b0d6](https://github.com/bhgv/flashVM/actions/runs/43668162604)
завершился успешно во всех шести jobs: AMD64 plain/TSan, ARM64, RV64/QEMU,
DIS64 Sys AMD64/ARM64. Это закрывает прежний startup namespace blocker;
physical RV64 hardware acceptance по-прежнему не заявляется.

Sys.open(OWRITE) теперь открывает existing file без создания/усечения и без read
permission, с позиции ноль. Новый port mode wo использует native O_WRONLY;
прежние w/wx/rw не меняют семантику. [Контракт RU/EN](WRITE-OPEN.md).
Guest fixture проверяет actual 0222 opens/read refusal, запись ABCDE → XBCZE,
seek, missing/directory/OTRUNC refusal, bound 0444 refusal и zero live file owners.
Native anchor fixture проверяет direct/relative opens и fdopen failure cleanup.
Локально: полный AMD64 dynamic wide 70/70, static host-unit 29/29;
расширенный guest fixture прошёл static/dynamic × C/JIT × оба poll (8 runs).
Изменённые POSIX порты дополнительно компилируются как C89. Новая
архитектурная CI относятся к этой ревизии отдельно от указанного прежнего CI.
Windows native-path implementation не проверена; Windows anchors unsupported.
Guest credentials, полная provider VFS и sandbox остаются следующими этапами.

## 2026-11-15: проверка host rwx и cross-build anchor gate

Добавлен mandatory workload namespace-permissions: реальные chmod fixtures,
успешные read/create, отказ create поверх readonly файла без изменения bytes,
проверка mode 0444, отказ open/stat без directory x через bind, затем unmount.
Static/dynamic AMD64 проверены в interpreter/JIT × fast/locked poll (8 runs).
Это проверка kernel permissions host user; guest credentials и sandbox не заявлены.
Sys.open OWRITE пока неподдержан и не используется как доказательство permission
refusal. Root host user явно отвергается тестом, а не выдаётся за успешную проверку.

Anchor-test target теперь создаётся и при CMAKE_CROSSCOMPILING. Его наличие
и выполнение проверены cross-конфигурацией AMD64; это проверка CMake gate,
не RV64 execution. Список native wide содержит 70 workloads. Полная новая
архитектурная матрица ещё не выполнена; публикация локальных изменений
заблокирована automatic review до явного разрешения push в master.

## 2026-11-15: RV64/QEMU и единый корень directory anchors

CI на `5e573d2` завершил пять jobs успешно, но RV64 отказал 69/69 workload
до guest startup: `cannot initialize startup namespace`. RV64 job теперь линкует
статические ELF и не задаёт QEMU_LD_PREFIX: sysroot redirection абсолютного `/`
несовместим с последующим host-relative openat от этого directory descriptor.
Отдельный anchor-port gate перед workloads проверяет root-to-cwd lookup,
rename, concurrent opens/removes и cleanup. Runtime lookup не обходит anchors.

Локальный AMD64 static build: 29/29 host-unit; wide 68/69 до исправления
порядка private probe link. Единственный отказ был undefined fesetenv в jit-fp:
probe object стоял после libm. Теперь object идёт перед archives; весь jit-fp
с actual RX faults и omission controls повторён успешно в static и dynamic builds.
Остальные 68 workloads
прошли, включая namespace/remove/load/debug, directory anchors и network proxy.
Это локальная проверка static configuration, не завершённая RV64 приёмка.
Новая архитектурная CI должна подтвердить target execution; до её завершения
исходный RV64 blocker не считается закрытым.

## 2026-11-15: удаление файлов через native namespace

Sys.remove выполняет один retained parent/name unlink либо empty-directory removal
вне namespace mutex и managed admission. Result publication guarded; эффект не
повторяется. Открытые FD/Module code живы после unlink; новый load image fails.
Четыре owners проверяют real rename/decoys, file bindings, shared result cell,
одного winner и zero live anchors. Port тестирует final/dangling symlinks,
empty/full dirs и восемь OS owners.69 mandatory workloads включают namespace-remove.
[RU](NAMESPACE-REMOVE.ru.md), [EN](NAMESPACE-REMOVE.md),
[пример](../examples/namespace-remove/README.ru.md).
Windows relative backend, full Chan/providers и union по-прежнему не заявляются.

## 2026-11-15: namespace в debugger по запросу

Команды namespace/namespace-read показывают root/cwd/import и все bindings
удержанного guest. Полные UTF-8 paths читаются bounded hex pages с mutation
revision; shared peer rebind делает старые pages недействительными. Нет новых
постоянных таблиц статистики, allocation/I/O/GC traversal; читается существующее
native state под mutex. Frozen source продолжает владеть своей группой.
68 mandatory workloads включают namespace-debug. Полные Chan/provider graphs,
FD/env/credentials и non-Linux adapters остаются работой.
[RU](NAMESPACE-DEBUG.ru.md), [EN](NAMESPACE-DEBUG.md),
[пример](../examples/namespace-debug/README.ru.md).

## 2026-11-15: удержание directory resources при rename/rebind

Root/cwd/import/directory bindings удерживают native handles; in-flight plans
сохраняют их до окончания I/O и освобождают при native unwind. FORKNS копирует
refs; NEWNS удерживает cwd как root. Четыре native owners проверены с настоящим
rename и подменой старого имени, чтением/create/import, независимыми cursors,
covering rebind и identity-filter unmount. Контроль реально читает подмену;
shutdown требует zero live anchors. Отдельно восемь OS owners проверяют relative
open, exclusive create и закрытие FD при позднем отказе fdopen без truncate.
Обязательный native-набор теперь67.
[RU](DIRECTORY-ANCHORS.ru.md), [EN](DIRECTORY-ANCHORS.md),
[пример](../examples/directory-anchors/README.ru.md).

File binding держит parent+basename; полный Chan/mount-parent walk, providers,
FD/env/credentials и non-Linux adapters остаются работой. Windows anchor backend
явно UNSUPPORTED; macOS/Android POSIX ещё требуют физической приёмки.

## 2026-11-15: namespace загрузчика и identity открытого образа

Файловый `load` разрешает namespace один раз до cache и native release.
Cache/open/parser получают frozen absolute host path; debug labels показывают
точный source. `chdir` обновляет import directory группы; FORKNS копирует его,
NEWNS сбрасывает в `/`. Bundled names теперь обычные file bindings, без
специального обхода namespace в `kopen`.

Исправлен hosted `kdirfstat`, прежде возвращавший нулевые identity/time.
Теперь metadata берётся с открытого файла. Проверены реальные cold/warm opens,
peer rebind/load во время native ожидания, atomic replacement с одинаковыми
length/time, старый живой Module и отказ cache при слишком широкой identity.
Omission zero metadata воспроизводит конкретный stale code со штатным retirement.
Обязательный native-набор теперь66. [RU](MODULE-NAMESPACE.ru.md),
[EN](MODULE-NAMESPACE.md), [пример](../examples/namespace-load/README.ru.md).

Публикация images — immutable files с atomic replacement; in-place edit при
неизменных секундных metadata не гарантирует обновления cache. Chan/provider
handles, полная revision/content identity, union/mount и остальные группы
окружения остаются следующими блоками. Исполняемые не-Linux adapters не заявлены.

## 2026-11-14: наследуемые native namespace

Добавлены `Sys.pctl(0/FORKNS/NEWNS, nil)`, `chdir`, `bind(MREPL)` и `unmount`.
`spawn` разделяет группу; FORKNS копирует, NEWNS берёт resolved cwd за новый
корень. Open/create/stat разрешают группу вызывающего; уже открытые FD стабильны.
Нет глобальной exclusive-секции. [Точные границы RU](VFS-NAMESPACE.ru.md),
[EN](VFS-NAMESPACE.md), [пример](../examples/namespace/README.ru.md).

Это host-path routing, ещё не Chan/provider VFS и не sandbox: symlinks,
rename/unlink identity, module-loader namespace/cache, union/mount providers,
FD/env/credential groups и dynamic extensions остаются работой следующих блоков.
Обязательный native-набор теперь65 с `namespace`. Linux x64 проверяется локально;
ARM64/RV64 требуют новой CI. Исполняемые Windows/macOS/Android adapters этим
блоком не перенесены; общее ядро namespace использует portable mutex API/libc.

Ниже сохранена история предыдущих checkpoints; старые количества workloads
и завершённые CI относятся к соответствующим прежним ревизиям.

## Текущий checkpoint: typed network proxies

[Trusted network owners](../remote/NETWORK-OWNERS.md) соединяют две реальные VM
через заранее установленный доверенный поток, включая loopback TCP. Проверяются
восемь клиентов, исходные variable/function owners, actual ref aliases, epoch,
cache/replay, разрыв после callback admission, malformed/truncated frames,
retirement callers, idle broker expiry и полный terminal root/IO cleanup.

Это не новый Dis64 loader/ABI, не owning dial/listen и не distributed GC.
Authentication и уникальный 128-bit connection epoch предоставляет transport/
launcher. Текущий manifest содержит 49 native workloads, 7 omission controls и
26 host-unit tests; под TSan добавляется контроль старой late-reply гонки.
Сетевой набор отдельно прошёл в C/JIT и обоих poll paths обычно и под TSan.
Расширенный полный TSan выявил raw scalar return race в Processctl.pid; она
исправлена guarded publication, с отдельными четырьмя writers/branch readers и
actual-race omission control. См. [точную границу builtin helper](BUILTIN-SCALARS.md).

Предшествующая матрица [6aedab6a](https://github.com/bhgv/flashVM/actions/runs/43439432556)
завершилась success во всех шести jobs: AMD64 plain/TSan, ARM64, RV64/QEMU и
два Dis64 Sys jobs. Она подтверждает прежние 48 workloads, не новые network cases.
QEMU не заменяет physical RV64 ordering/performance acceptance.

## Политика master

Активная разработка перенесена в `master`. Native-многопоточность без глобального
эксклюзива — обязательный основной и единственный поддерживаемый режим, включая
диагностические параметры. `--managed-workers` с любым значением, `--gc-inline`
и `--sequential-ui` отклоняются до старта VM. `--native-spawn` — совместимый явный
псевдоним штатного режима. Старые reference-ветви/fixtures сохраняются как история,
но штатная регистрация CTest и CI их не запускает. Локальный mutex poll остаётся
конкурентным вариантом polling, не остановкой мира.

Перенос не означает удаление всех исторических реализаций из upstream snapshots;
их запрещено реактивировать как режим исполнения или проверки.

## Приёмка первоначального переноса master

Перенос выполнен коммитом [`93a5154`](https://github.com/bhgv/flashVM/commit/93a5154a496c1f82de2f880ec999b4fa26097c15)
с сохранением истории проверенной ветки. До переноса `master` был её предком;
обновление выполнено fast-forward без force-push. Дальнейшие изменения публикуются
в `master`, workflow `.github/workflows/native-disvm.yml` отслеживает `master`.

Новый состав приёмки: 46 native-нагрузок, 4 native omission controls и 21 чистая
проверка host ABI/guards/генератора. Под TSan добавляется контроль старой гонки.
Локально весь состав прошёл **71/71 обычно и 72/72 под TSan**. Диагностические
`--managed-owner-probe` и `--native-queue-probe` дополнительно проверены на четырёх
постоянных гостях со строгой GC-политикой.

В [CI master](https://github.com/bhgv/flashVM/actions/runs/43389832715) AMD64 и ARM64
прошли по **46/46 native-нагрузок и 21/21 host-unit проверок**. AMD64 TSan и
RV64/QEMU ещё выполняются на момент этой записи; незавершённые jobs не считаются
успешными. Прежняя матрица 44/44 ниже относится к состоянию до переноса.

Добавлены проверки обязательного default-профиля и native-конструкторов на
40 постоянных владельцах. Старый shared/reference constructor oracle больше не
исполняется; от него используется только неизменный текст Limbo payload.
Исторические mixed/reference регистрации не подключаются. Числа 71/72 относятся
к новому составу, а не повторению старых 100 CTests. Чистые проверки Sys guard,
выбора платформы и генерации сохранены без запуска reference VM.

В этом переносе изменяется профиль запуска и приёмка, а не алгоритмы GC или
mailbox. Upstream snapshots не менялись. Изменённые исходники — не более 408 строк.

## Решение по этапу

**Рабочая native-многопоточная база для поддержанного консольного профиля
DisVM/Limbo реализована и прошла текущую приёмку. Можно переходить к прикладным
задачам на этой базе.** Дальнейший поиск гонок — сопровождение и проверка
конкретных нагрузок, а не бессрочное условие включения многопоточности.
Это решение ограничено описанным профилем; оно не объявляет весь исходный
проект, все расширения или каждое приложение полностью проверенными.

- Каждый `Prog` исполняется своим постоянным системным потоком с собственными
  живыми TLS-регистрами. Переноса гостя между исполнителями и восстановления
  рабочих регистров из `Prog.R` при пробуждении нет.
- Общий NativeWorld exclusive и остановленные проходы GC запрещены. Fast,
  Cycles и Duke работают конкурентно. Сохраняются локальные замки конкретных
  очередей, каналов, таблиц, дескрипторов и метаданных; это не режим без mutex.
- Запросы между потоками идут через mailbox. Исправлены подтверждённые ошибки
  публикации состояния/регистров, запоздалых ответов отмены и общей памяти
  уведомлений завершения детей.
- Обычный консольный запуск выбирает native-профиль по умолчанию. Для явного
  выбора при запуске приложения: `disvm --native-spawn --jit program.dis`.
  Выбор однопоточного reference-режима удалён.
- Неподдержанный legacy-путь не получает скрытый эксклюзивный fallback:
  попытка остановленного collector-пути явно завершается с кодом 70.

## Завершённая приёмка до переноса в master

Production-код: [`74fbab4`](https://github.com/bhgv/flashVM/commit/74fbab4f2aa066ba850554f9bcb5607d6832c8c0).
Последующий `c6159b2` уточняет комментарий и документацию; исполняемый C-текст
не меняется. [Полный завершённый CI](https://github.com/bhgv/flashVM/actions/runs/43384574676):

| Конфигурация | Результат |
| --- | --- |
| AMD64, системные потоки, C/JIT | 44/44 |
| AMD64, TSan, C/JIT | 44/44 |
| ARM64, настоящее оборудование runner, C/JIT | 44/44 |
| RV64, целевые ELF и JIT под QEMU | 44/44 |
| Дополнительный AMD64 constructor/channel oracle | Пройден обычно и под TSan |

Локально: 44/44 обычных нагрузок, 44/44 под TSan, 100/100 расширенных
native/boundary CTests и финальные 5/5 TSan-проверок с отрицательными контролями.
В workload входят несколько VM-запусков; это число сценариев, а не потоков.
TSan инструментирует C runtime, но не сгенерированные машинные инструкции JIT.

Проверяются строгая GC-политика, постоянные владельцы, результаты приложений,
передача ссылок/корней, каналы и I/O, отмена, retirement и join. Такой результат
не является доказательством отсутствия всех возможных гонок или измерением
ускорения. Список переносимых нагрузок: `native_arch_cases.py`, профиль `wide`.

## Отдельные следующие работы

| Работа | Граница текущего результата |
| --- | --- |
| RV64 на настоящем многопроцессорном железе | QEMU проверяет функциональность и ABI; hardware ordering/throughput ещё не проверены |
| Сторонние C callbacks/finalizers | Требуют собственного контракта владения и потоков; произвольные расширения не сертифицированы |
| GUI/editor и прикладные службы вне console-матрицы | Нужна приёмка соответствующего приложения/host |
| Оптимизация JIT и масштабируемости | ARM64/RV64 имеют настоящие native tail-входы к проверенному C executor; оптимизированное inline-покрытие всех opcode и ускорение не заявлены |
| FlashVM | Отдельный следующий проект; текущие изменения относятся к DisVM/Limbo |

Новые воспроизводимые дефекты поддержанного профиля исправляются как конкретные
ошибки. Сама возможность находить новые ошибки не удерживает остальные задачи
до завершения неограниченного аудита всей кодовой базы.

## English scope

The dedicated native-thread implementation is a usable, tested baseline for the
supported console DisVM/Limbo profile. All four architecture CI jobs completed
44/44; the local broader gate completed 100/100. This checkpoint closes baseline
implementation/acceptance work for that profile and permits application work.
It is not blanket certification of arbitrary extensions, GUI hosts, every VM
path, RV64 hardware ordering, JIT instruction instrumentation or speedup.
The remaining work above has separate, concrete acceptance targets.


2026-11-14: mandatory workload list now62 includes retained-debug and
jit-quarantine. Selected local ordinary/TSan regressions do not establish new
whole-matrix completion. Previous cf066b75 and47665a architecture CI runs
43605594611/43605884568 completed successfully. / Обязательный список теперь62;
новые retained-debug/jit-quarantine требуют своей полной CI-матрицы. Предыдущие
указанные CI завершились успешно.


2026-11-14: list63 adds jit-signal with actual RX traps, retained native CPU
context and unsafe-path refusals. Prior e123a54d CI43617004556 failed retained-debug
socket mode:0777 & ~0077 gives0700, not0600. Creation now uses0177 and a host-independent
mask/restoration probe; full AF_UNIX execution remains a CI check on this socket-denied
host. / RU: Список63; исправлена маска socket до0600, добавлена проверка umask
без AF_UNIX. Новую полную архитектурную матрицу нельзя считать пройденной заранее.


2026-11-14: prior CI43622919835 passed all six jobs including native ARM64 and RV64
QEMU. New list64 adds jit-fp (base FP/SIMD kernel copies, REAL RX faults and fenv
restoration); new matrix completion remains a separate observation. / Предыдущая
матрица прошла; новый FP-блок требует собственной архитектурной проверки.
