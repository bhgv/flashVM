# v0.1673 — Публикация ALT и рабочий стол MovieClip

Ветка experiment закреплена на **63abd22fc86b353b0140d238811a6dd1560a658b**.
Dis **793**, bootstrap **277**, всего **950 уникальных выбранных файлов**:
точные байты Git и хеши манифестов проверены. Импортирован порядок захвата/отмены
выбора ALT до записи перекрывающихся payload/output. Матрицы C/RX проверяются
с ASan/UBSan. Обратная проверка не выявила нового общего дефекта для отправки;
прошлые исходящие исправления уже находятся в experiment. Сервисы рабочего стола
Limbo и адаптеры владельца FlashVM остаются в этом проекте.

Core 210/210; runtime 239/239; заключительные проверки владельцев/рисования 5/5.
[Серия](../docs/releases/v0.1673.md) · [ALT](upstream/libinterp/tests/README.alt-publication.md) · [Рабочий стол](../ide/DESKTOP.ru.md).

# v0.1672 — Числовой benchmark и исходящее исправление GC-idle fixture

Итоговый pin experiment: **573eb1337fd621b6e51b89d16dd86427c9376452**. **791** файл
Dis, **277** bootstrap, **948 уникальных выбранных файлов** совпадают с Git/hashes.
Импортирован 75bc786: центрированная статистика, таймеры и чтение отчётов.
Полный core воспроизвёл отсутствие настоящего tickspan в сборке gc-idle fixture.
Исправление двух файлов сначала проверено/опубликовано в Inferno, затем импортировано
побайтно. Новый и прежний benchmark waiter replay проходят; производственная VM/GC
этим исходящим исправлением не меняется. Эксклюзивные файлы host, канонический FIR
и политика IDE остаются в FlashVM. Итоговые проверки: core 208/208, runtime 235/235
и 12/12 focused на неизменных производственных VM inputs; compiler service 3/3
в обычной и санитайзерной сборках.

[Релиз](../docs/releases/v0.1672.md) · [Числовые проверки](upstream/libinterp/tests/README.bench-numeric.md) · [GC fixture](upstream/libinterp/tests/README.gc-arithmetic.md).

# v0.1671 — Общая проверенная выборка для отладчика и профилирования

Закреплён experiment **51d18eaea7fb1aa724fcbb5e0aaa92f7c1d1a4fa**: **787 файлов Dis**,
**277 bootstrap**. Все **944 уникальных выбранных исходника** совпадают с Git
Inferno и SHA-256 manifests. Обычный executor, emu/os debugger/coverage и benchmark
используют общий vmfetch.h. WORD32/64 матрицы исполняют production decoder,
проверяют уведомления, callback yields, native handoff и владение при reset.

Host создаёт отдельное TLS-представление общего заголовка. Проверенная выборка
предшествует GC poll/quota и исполнению C/AOT/JIT. Генерация проверяет действительные
build inputs, CMake отслеживает заголовок. Upstream-снимки не модифицируются.
Обратная сверка не нашла нового общего дефекта. Предыдущее исходящее исправление
Math уже присутствует в experiment; частных расхождений общего кода нет.
Изменения панели IDE/NC64 остаются здесь. Проверки: core 207/207, runtime 230/230,
bootstrap 26/26. Более поздние изменения upstream требуют отдельного review.

[Релиз](../docs/releases/v0.1671.md) · [Тесты циклов](upstream/libinterp/tests/README.instrument-loops.md) · [Панель сборки](../ide/BUILD-PANEL.ru.md).

# v0.1670 — Двусторонняя синхронизация Math, fetch, AOT и SBL

Final experiment pin: **34d019f22b80d01385db737e13b20c0ff443aa8b**, **781 Dis / 277 bootstrap files**.

Incoming changes bound optimizer storage, make SBL Type output iterative, validate symbol metadata, and check normal C/AOT/JIT instruction fetch. The selected Dis-to-C reference includes its native-safe scalar/address tests; it is not an installed NC64 IDE execution backend.

Outgoing shared fix: [Math module-local primitive descriptors](https://github.com/bhgv/Inferno-OS-bhgv/commit/34d019f22b80d01385db737e13b20c0ff443aa8b). The previous FlashVM-only Math adapter is retired. Shared guards accept compatible pointer-free local layouts, preserve wrong canonical primitive rejection (including big at equal native widths), and retain error order and full-width bounds. Parent WORD32/64 sources reproduce the rejection; new sanitizer predicates and previous codec/vector matrices pass.

The host safepoint now follows checked fetch and precedes quota/AOT/decode. NC64 fast dispatch uses the captured callback, avoiding a Jit reread after enter. Source-view regeneration verifies both placements and the shared Math source. No upstream source is patched in place here.

Final checks: core **206/206**, runtime **224/224**, bootstrap **26/26**. Both snapshot manifests verify, and all **938 unique selected source files** match the published Inferno commit byte-for-byte; no unpublished common delta remains.

[Release](../docs/releases/v0.1670.md) · [Math contract](upstream/libinterp/tests/README.math-local-types.md) · [Checked fetch](upstream/libinterp/tests/README.xec-fetch.md) · [AOT reference](upstream/utils/dis2c/README.md).

# v0.1668 — Проверяемые области видимости

Pinned experiment revision: **36d5228f5e46a594a8b983429e16fb69eb07a7e9**, **752 Dis / 270 bootstrap files**.

Imported exact upstream lexical-scope bounds, label-growth guards and iterative global binding, including the frozen reference and scope-binding regression. Bootstrap registers the new test: **24/24** pass. Both manifests retain per-file hashes. No additional shared defect was found requiring an outbound Inferno patch; FlashVM-specific FIR/source changes remain here. Later upstream revisions are separate work.

[Release](../docs/releases/v0.1668.md) · [Scope test contract](upstream/limbo/tests/README.scope-binding.md).

# v0.1667: итоговый pin — коммуникации и tuple aliases

Итоговый pin: **10590495858d4c3c4dca907e2e321c9f74ffa6b9**, **748 файлов Dis / 266 bootstrap**.
Перед публикацией добавлено следующее обновление experiment: нерекурсивные
rewritecomm и tuple/name alias queries. Сохранены порядок callbacks, отложенное
чтение изменяемых ячеек операндов и возврат захваченного поддерева. Проверяются
изменения соседнего операнда из callback, включая первоначально nil-ячейку.
Сравнение с замороженным неглубоким оракулом и 100000-уровневые проверки выполняются
для WORD32/WORD64; bootstrap проходит 23/23. Позднейшие upstream-коммиты не меняют
проверяемый pin этой серии. Нового общего дефекта для обратного коммита не найдено.
[Релиз](../docs/releases/v0.1667.md) · [Контракт теста](upstream/limbo/tests/README.comm-alias-walks.md).

## Предыдущий pin этой серии

# v0.1667 — Проверяемые использования деклараций и временные удержания

Pin: **11e277299395ec75ea331c6ed1f93016b9c059c2**, **744 файла Dis / 262 bootstrap**.
Взят следующий после 577940f коммит experiment: общий проверяемый счётчик
Decl.refs/Decl.tref применяется ко всем оставшимся прямым изменениям в проходах
компилятора. Политика повторного использования/обнуления временных значений
сохранена; fatal не обещает rollback уже изменённых метаданных.

Новый production-тест reference-uses и обновлённые зависимые fixtures сохранены
побайтно. Новый драйвер включён в bootstrap. Прошли core 202/202 и bootstrap
22/22; обе выборки сверены по SHA-256. Нового общего дефекта для обратного
коммита не найдено. Наши группировка FIR и изоляция IDE-fixture принадлежат
FlashVM и в Inferno не отправляются. [Релиз](../docs/releases/v0.1667.md).

## Предыдущие контрольные точки

# v0.1666 — GC, native I/O и нерекурсивные метаданные Limbo

Текущий pin: **577940fa699d42593db7c680c5a0aeceeeb04e09**, **741 файл Dis / 259 bootstrap**.
Перенесены семь коммитов experiment после 50059a3c: насыщаемая арифметика бюджета/
обратной связи GC и ожидание завершённых unsigned-эпох; проверка полного long-ответа
I/O до сужения; нерекурсивные parent-Type, диагностика типов, печать C-stub, обходы
деклараций/tuple; проверяемые счётчики ссылок и стабильный порядок локальных групп.
[Точный список коммитов](SYNC.md).

Новые проверки исполняют настоящие production-функции с оракулами, включая ширины
WORD32/WORD64. Пять драйверов добавлены в bootstrap, четыре — в core; оба снимка
проверены по SHA-256. Прошли core 202/202 и bootstrap 21/21. Отдельные emu/os тела
нужны тестам: полная ОС и GUI/security-службы не импортированы. Необязательных
зависимостей эмуляции чужих инструкций здесь нет; аппаратная ARM64/RV64 не заявляется.

Нового общего дефекта для обратной публикации не обнаружено. Ранее отправленные
исправления остаются предками выбранного pin. Изменения нашего Coco/R/FIR/SWO
компилятора принадлежат FlashVM; ветка Inferno experiment этой серией не изменена.
[Релиз и проверки](../docs/releases/v0.1666.md) · [Числовые условия](../fir/v1/doc/NUMERIC_LOGIC.ru.md).

## Предыдущие контрольные точки

# v0.1657 — Синхронизация dial/file2chan и связывание алиасов

Текущий снимок: **0ac9d293660a718cb3dbc3472e9c40d6b31cca8e**, **683 выбранных файла**.
Взяты 275bc64 (границы dial/counts, относительные clone-пути) и 3ba10d2
(сохранение file2chan-кадров/результатов и владельцев). Наше обратное исправление
[0ac9d293](https://github.com/bhgv/Inferno-OS-bhgv/commit/0ac9d293660a718cb3dbc3472e9c40d6b31cca8e)
отвергает неполные/завышенные записи команд до открытия data и передачи control.
Полные тела обоих dial-портов исполняются с ASan/UBSan.

Новый проход алиасов Coco/R принадлежит нашему компилятору FlashVM. В Inferno
алиасы уже существуют: обратно передано общее исправление dial, а не наш
frontend или Flash-расширения. [Релиз](../docs/releases/v0.1657.md).

## Предыдущие состояния

# v0.1656 — Двусторонняя синхронизация Sys и проверки массива pipe

Текущий снимок: **9ee638bbfaff421164e2e7fb1d68e01089720c39**, **677 выбранных файлов**.
Взяты шесть коммитов Sys после 9a87d74 вместе с регрессиями. Наше подтверждённое
исправление типов элементов pipe опубликовано обратно в experiment:
[9ee638bb](https://github.com/bhgv/Inferno-OS-bhgv/commit/9ee638bbfaff421164e2e7fb1d68e01089720c39).
В emu и os больше не начинается доступ к указательным слотам массива скаляров.
Расширения any/table/проекций FlashVM в Inferno не переносились. Новых JIT-backends
во входящем диапазоне нет; прежние выбранные проверки native/compiler проходят.

Наш отдельный NC64-хост сохраняет двенадцать кадров Sys, проверяет полные результаты
и прежних владельцев. Прошли runtime 216/216, итоговые целевые 26/26, десять Sys и
пять JIT/compiler-драйверов. Guard/pipe-тесты проходят ASan/UBSan; недоступная
проверка процессов LSan отключена.
[Контракт RU](runtime/SYS_CAPTURE.ru.md) / [EN](runtime/SYS_CAPTURE.md) ·
[Релиз](../docs/releases/v0.1656.md).

## Предыдущие состояния

# v0.1653 — Sys frames/owners and mapped capture refresh

Current pin / Текущий снимок: **9a87d74eb4f47cf7b8ea35c804f86c7c20e40d36**.
Exact selections / Точные выборки: **662 core/reference/library + 207 bootstrap files**.
Incoming changes validate complete Sys UTF/tokenizer/public-format frames, packed
views and owners before side effects, and capture mapped-copy operands across
destructor callbacks. Original C, complete AMD64 JIT and WORD-width tests retained.
Full core 248/248 at 047e0dc plus final affected 9a87d74 refresh 6/6 passed.

Bidirectional review / Двусторонняя сверка: experiment retains our previous
reverse fixes, including 73a6b5a. No new shared defect was found for a reverse patch;
AS2/FIR/IDE changes are flashVM-owned and were not copied into Inferno.
[RU/EN release](../docs/releases/v0.1653.md) ·
[AS2 → FIR → SWO → SWF](../compiler/as2/view/README.md).

## Previous checkpoints / Предыдущие состояния

# Обзор модулей, обратное исправление Regex и оптимизатор bootstrap

Итоговый снимок: **320e6581e425c014f8a9ba313352b664925a709f**.
Точные выборки: **633 файла ядра/эталонов/библиотек + 207 bootstrap-файлов**.
Входящий cfb91bc ограничивает стек обходов графа оптимизатора и расширяет счётчики
использований/определений битовых карт. Регрессии графов и WORD32/64 включены в тесты.

Обзор воспроизвёл ошибку Unicode в Regex: внутренние теги и конец ввода совпадали
с U+10000…U+1000B. Исправление проверено и опубликовано обратно в Inferno как
**320e658** с автором/коммиттером bhgv, затем импортировано побайтно. Regex теперь
собирается обычным Limbo-модулем в нативной и отдельной сборках библиотек.
Runtime/editor SDK получают исходник, заголовок, байткод и символы; проверка
перенесённых SDK заново собирает библиотеку/клиента и запускает оба режима
с квантами 1/2048. [Обзор](MODULE_AUDIT.ru.md) ·
[Регрессия Inferno](upstream/appl/lib/tests/README.regex-unicode.md).

Прежняя неизменённая библиотека отвергает U+10000; исправленная проходит 85 проверок
литералов, захватов и классов в каждом из четырёх запусков. Это ещё не синтаксис
regexp-срезов во фронтенде, отдельный regexp-JIT и не законченный /prog-отладчик.
Обход с одним именем в компиляторе 0.47 остаётся отдельным Coco/R-расширением FlashVM.
Нативный хост здесь AMD64; физический ARM64 и полный стандартный ABI проверяются
отдельно. Оба снимка включают прежние обратные исправления.

---

# Границы ALT и отмена регистраций при ошибке обмена

Итоговый снимок: **1bd39b084006caabda0b712d3b011975c5241814**.
Точные выборки: **623 файла ядра/эталонов + 206 bootstrap-файлов**. ALT/NBALT
отвергают переполняющийся заголовок и неполный WORD результата до регистрации.
Защищённый выбранный обмен при ошибке отменяет регистрации данного alt перед
повторным выбрасыванием, сохраняя чужих ожидающих и FIFO. Уже выполненные операции
с данными, пробуждения и записи выбора/индекса не откатываются.
[Контракт Inferno](upstream/libinterp/tests/README.alt-power.md).

Исходные ASan/UBSan NDEBUG-матрицы прошли **243 860 случаев очередей через C +
243 860 через полный JIT**, а также **599 408 случаев передачи данных**.
Добавлен core-тест полных нативных хвостов ALT/NBALT для AMD64/ARM64-хостов.
Полный прогон 183 runtime-тестов использовал 9283015 с компилятором 0.46; поздний
импорт пересобирается и отдельно проверяется на каналы, ошибки, скалярный компилят
и свежесть генерации. Прежние обратные исправления сохранены; новых дефектов
для обратной отправки не найдено. Оптимизация остаётся только во FlashVM.
Нативный хост здесь AMD64; физический ARM64 и полный нативный ABI — отдельная работа.

---

# Захват корней очистки стека и упакованные диагностические дескрипторы

Проверенный снимок Inferno experiment: **92830151d9b130e6fea1f122c688165eee743fea**.
Точные выборки сохраняют **622 файла ядра/эталонов + 206 bootstrap-файлов**.
Очистка стека заранее захватывает родительское расширение и корневой Type до
деструкторов отображённых владельцев. Счётчики указателей упакованного Type и
диагностические битовые карты читаются побайтно. Изменение сохранённых EX/TR
деструктором не перенаправляет остаток очистки и не теряет владельцев корня кадра.

[Контракт Inferno](upstream/libinterp/tests/README.stack-frame-contract.md).
В исходном репозитории нативная WORD32/WORD64-матрица прошла **196 608 случаев**
под ASan/UBSan, NDEBUG и строгими предупреждениями. Это не общий контракт изменения
Type из callback и не проверка на физическом ARM64. Прежние обратные исправления
сохранены; новых дефектов для обратной отправки не найдено. Оптимизация приватных
скалярных записей компилятора 0.46 остаётся только во FlashVM.

---

# Обновление точной проверки JIT-кэша и образа кода

Итоговый проверенный снимок: **6c468ef5ecf203c2ca4b0e7eef435a145399b3d0**.
Точные выборки: **622 файла ядра/эталонов + 206 bootstrap-файлов**. Повторное
использование decoded-leaf-кэша требует правильного порядка входов/хвостов
опкодов, точного совпадения байтов, размера и служебных функций компилятора.
Адрес внутри допустимого диапазона, но от другого опкода, больше не принимается.
Проверка использует ограниченный приватный буфер при повторном использовании,
а не на каждой исполняемой инструкции.
[Контракт Inferno](upstream/libinterp/tests/README.cache-bindings.md).

Пройдены исходные нативные ASan/UBSan WORD32/64-матрицы диспетчеризации, кэша и
интеграции, а также проверки ARM64-компилятора на хосте. Полный прогон 179 runtime-
тестов использовал предшествующий снимок c2a08e3 с компилятором 0.44; после позднего
импорта runtime пересобирается и повторяются целевые проверки диспетчеризации,
владения и исполнения компилята. Прежние обратные исправления сохранены в Inferno;
новых дефектов для обратной отправки не найдено. Расширение языка остаётся только
во FlashVM.

---

# Выравнивание Heap и защиты владельцев в приоритетных JIT

Снимок Inferno experiment: **c2a08e302b766b9299a79395ce6e67302de41146**.
Точные выборки: **621 файл ядра/эталонов + 206 bootstrap-файлов**. MOVP, SELF,
HEADP и TAIL отвергают невыровненные идентификаторы Heap до обращения к метаданным
владельца; упакованные ячейки операндов поддерживаются. Быстрые пути AMD64/ARM64
проверяют выравнивание до удержания, маркировки и публикации владельца. Отличающийся
прежний владелец назначения по-прежнему безопасно передаётся обработчику C.

[Контракт и матрицы Inferno](upstream/libinterp/tests/README.owner-preflight.md).
Пройдены нативные ASan/UBSan WORD32/64 и гостевые матрицы AMD64/ARM64, включая
полные JIT-входы с настоящим C fallback. Новых дефектов для обратной отправки
не найдено; прежнее обратное исправление 51d44da сохранено. Семантика отложенного
шага компилятора 0.44 остаётся только во FlashVM. Это не означает завершения
64-битного ABI компилятора/кадров или проверки на физическом ARM64-хосте.

---

# Обновление проверки адресов обработчиков исключений

Проверенный снимок Inferno experiment: **d229914a31f9d92bc7fb114033bcce6a869cd898**.
Точные выборки содержат **621 файлов ядра/эталонов + 206 bootstrap-файлов**.
Явные и стандартные адреса обработчиков проверяются до диагностики, раскрутки
кадров и публикации владельца исключения. Некорректная цель не скрывает следующий
правильный обработчик. Для интерпретатора проверяется целая инструкция; для
скомпилированного кода сохраняются байтовые смещения и исключаются переполнение/H,
без выдуманного размера JIT-выделения, которого модуль не хранит.

В исходном репозитории emu/os WORD32/WORD64 проходят **2 104 000 случаев** под
ASan/UBSan с NDEBUG. [Контракт и команды](upstream/libinterp/tests/README.exception-frames.md).
Прежнее обратное исправление **51d44da** уже находится в Inferno; новых дефектов
для обратной отправки в этом обновлении не обнаружено. Семантика инициализации
компилятора 0.43 остаётся только во FlashVM. Полный прогон 179 runtime-тестов
завершён до этого импорта; новый снимок пересобирается и отдельно проверяется
на исключения, публикацию владельцев, свежесть генерации и исполнение компилята.

---

# Финальное обновление публикации RAISE

Итоговый проверенный снимок: **4c94e2af2e8e2a927fdbb06c800ce5dd6453784f**.
Получены проверки метаданных RAISE и безопасная передача владельца исключения
в emu/os. Снимки: **620 файлов ядра/эталонов + 206 bootstrap**.
[Контракт исключений](upstream/libinterp/tests/README.exception-publication.md).
Обратное исправление **51d44da** включено и отмечено upstream.
Это обновление не меняет семантику языка/FIR FlashVM.

---

# Синхронизация переходов и обратное исправление диагностики

Inferno experiment закреплён на **51d44da4670567c7d38820ca0b1dfd8e3e4d895d**.
Снимки: **617 файлов ядра/эталонов + 206 bootstrap**. Получены проверки полных
целей CASE/GOTO/MOVPC и nil/H прямых переходов в C и приоритетных эмиттерах.
[Границы PC](upstream/libinterp/tests/README.pc-target-boundaries.md) ·
[Прямые переходы](upstream/libinterp/tests/README.direct-targets.md).

В исходном Inferno воспроизведена и исправлена ошибка сборки нативного драйвера:
leaf64-runtime теперь берёт production JMP/branchpc вместе с branchreal. Обратный
коммит **51d44da** опубликован в experiment; нативные WORD32/WORD64 матрицы UBSan
и обновлённая сборка ядра проходят. Новые native/guest проверки целей и готовности
прошли. Изменения языка/FIR FlashVM в Inferno не переносятся.

---

# Синхронизация упакованных карт GC

Inferno experiment закреплён на **381ecd2f8162bc02d9bda6cedfde9aad9fb8bbe6**.
Снимки содержат **610 файлов ядра/эталонов + 206 bootstrap**. Метаданные Type
и упакованные слоты читаются без предположений о выравнивании при GC/очистке
владельцев. Сохранённый кадр RET проверяется в C и эмиттерах AMD64/ARM64.
Новые тесты графов GC, карт и реальной очистки builtin сохранены побайтно.
[Контракт upstream](upstream/libinterp/tests/README.gc-packed-maps.md).
Выбранные проверки GC, карт, модулей, переходов, готовности и hosted-ссылок прошли.
Нового общего исправления для обратного переноса не найдено.

---

# Синхронизация целей CALL/MCALL

Inferno experiment закреплён на **51227cb0629bc6fd755d4d6ecad44f055ea56690**.
Снимки: **607 файлов ядра/эталонов + 206 bootstrap**. Перенесены проверки целей
CALL/MCALL до публикации перехода в C и быстрых путях AMD64/ARM64, с сохранением
непрозрачных PC-токенов старого JIT и проверкой геометрии эмиттера.
[Контракт и регрессии](upstream/libinterp/tests/README.call-targets.md).
Проверки нативных переходов, гостевых образов приоритетных архитектур и готовности
прошли. Нового общего исходящего исправления не найдено; расширения языка FlashVM
остаются отдельно.

---

# Переходы между модулями и компилятор 0.39

Inferno `experiment` закреплён на **45370abd78dd8b9339c962a544d2a933663139f1**.
Выбрано **606 файлов ядра/эталонов + 206 bootstrap**. Входящие общие исправления
сохраняют назначение LOAD и контекст модуля при callback, восстанавливают
состояние после встроенного MCALL по захваченному кадру и отклоняют ошибочные
SPAWN/MSPAWN до выделения памяти и публикации процесса. Новые upstream-регрессии
C/RX сохранены побайтно и включены в CTest ядра.
[Контракт upstream](upstream/libinterp/tests/README.module-transitions.md).

Нового общего дефекта для обратного коммита при сравнении не найдено.
Ячейки int/bool, постфиксные обновления int и for-in строк/снимков — расширения
исходного языка FlashVM 0.39 на существующих операциях FIR/Dis; в Inferno они
не переносятся. [Руководство](../fir/v1/doc/TYPED_LOOPS.ru.md).
Нативные регрессии здесь исполняются на доступном AMD64-хосте. Наличие тестов
компилятора/переходов ARM64 не означает завершённого переноса нативного ABI VM.

---

# Host extensions / Расширения хоста — v0.1649

Pinned Inferno remains **637d618e40fb6efb69cb0c147efb81981ee95c63**, unchanged.
Relative indexes and inclusive negative-end slices are FlashVM host/compiler
extensions. Source transformations retain exact checked anchors; upstream files
remain byte-exact. Snapshots: 560 core/reference + 174 bootstrap files.
[EN](../fir/v1/doc/RELATIVE_SEQUENCES.md) / [RU](../fir/v1/doc/RELATIVE_SEQUENCES.ru.md).

Broker metadata synchronization is not payload synchronization. The requested
same-operation reentrant lock is specified in [the contract](OWNED_OPERATIONS.md),
but is not claimed as implemented. Current VM ownership serializes C operations.

---

# Synchronization / Синхронизация — v0.1648

Checked Inferno experiment at **637d618e40fb6efb69cb0c147efb81981ee95c63**.
No new incoming commit followed the previous series. Earlier reverse fixes are
retained upstream. Exact snapshots: **560 core/reference + 174 bootstrap files**.
AMD64/ARM64 availability remains **166/173 guarded fast paths** and **173/173
complete decoded entries**; neither metric represents full operand semantics.

Compiler 0.31 adds hybrid table constructors through existing FIR and hosted
table services. Reused C99 source services recover after syntax/SSA-limit failures.
These language changes remain FlashVM-only; no upstream opcode, helper ABI or
container representation changes. Shared complete-dispatch, queue and readiness
checks passed. No new shared defect requiring a reverse patch was confirmed.

Новых входящих коммитов нет; прежние обратные исправления присутствуют.
Конструкторы и работа с их исходными позициями остаются в FlashVM.
[EN](../fir/v1/doc/TABLE_LITERALS.md) / [RU](../fir/v1/doc/TABLE_LITERALS.ru.md).

---

# Synchronization / Синхронизация — v0.1647

Final Inferno experiment: **637d618e40fb6efb69cb0c147efb81981ee95c63**.
The branch initially remained at 5b9bdcf; a final head check found 637d618 during
publication. It was imported and tested before completing this series.
Snapshots now retain **560 core/reference + 174 bootstrap files**.

AMD64/ARM64 retain **166/173 guarded native fast paths** and now provide
**173/173 complete decoded entries** through safe C tail bridges. These are
separate availability metrics, not complete semantic coverage or block JIT.
The imported shared executor, compiler span/cache checks and W^X publication
remain byte-exact upstream. Earlier reverse fixes are retained in experiment.

Our NC64 host routes both direct and complete fast callbacks through one policy:
classic CASE/CASEL/GOTO leaves decline into the NC64 C adapters, and only actual
fast successes increment native statistics. No Jit pointer is reread after a
callback. A standalone policy check covers both routes and negative nonzero
completion. This host-only adaptation is not sent to Inferno.

Входящий 637d618 импортирован; прежние обратные исправления сохранены.
Расширения языка и NC64-политика хоста остаются в FlashVM. Нового общего дефекта
для обратной правки не подтверждено. CVTCW/strtol Inferno не меняется.
[Compiler EN](../fir/v1/doc/TEXT_VALUES.md) / [RU](../fir/v1/doc/TEXT_VALUES.ru.md).
[Complete dispatch contract](upstream/libinterp/tests/README.complete-dispatch.md).

---

# Synchronization / Синхронизация — v0.1646

Checked Inferno experiment at **5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5**.
No incoming commit followed the previous series. Its reverse LeafLayout fix
remains published upstream. Exact snapshots: **554 core/reference + 174 bootstrap**.
Native availability remains AMD64/ARM64 166/173 with partial operand coverage.

Compiler 0.29 adds checked signed32 shifts in FlashVM-only Coco/R/FIR lowering,
reference execution and both direct Dis emitters. It uses existing branch/shift
kernels and their exception path; there is no upstream VM opcode or ABI change.
Selected shared shift-slot, native WORD32/64 and readiness checks passed.
No new shared defect requiring another reverse patch was confirmed.

Входящих изменений нет; прежнее обратное исправление остаётся в experiment.
Новые операции FIR и расширение исходного Limbo остаются в FlashVM.
[EN](../fir/v1/doc/INT_SHIFTS.md) / [RU](../fir/v1/doc/INT_SHIFTS.ru.md).

---

# Synchronization / Синхронизация — v0.1645

Inferno experiment: **5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5**.
No new incoming commit followed 6945acaf before this series. Expanded string JIT
checks found a shared fixture ABI drift: Python LeafLayout lacked the buffered
channeltransferentry callback present in C. The strict C size/offset checks
correctly stopped execution before code generation.

[Reverse fix 5b9bdcf](https://github.com/bhgv/Inferno-OS-bhgv/commit/5b9bdcfd188cd16846f8935b5c7d9074dcc89ee5)
adds the missing field in its exact position. Size/offset checks remain enabled;
no production opcode, ABI or acceptance guard was weakened. The fix was tested
in its owning Inferno checkout before publication and imported back unchanged.

В Inferno отправлено исправление описания структуры ctypes; производственный
код JIT не изменён. Снимки: 554 core/reference + 174 bootstrap файла.
Availability remains 166/173 per AMD64/ARM64, with partial operand coverage.
Compiler 0.28 string expressions and int bit operations remain FlashVM-only.
[EN](../fir/v1/doc/TEXT_BITS.md) / [RU](../fir/v1/doc/TEXT_BITS.ru.md).

---

# Synchronization / Синхронизация — v0.1644

Inferno experiment: **6945acafa0df51c7ee3e591b1665ddb40d9a27bb**.
Imported 87785ec: native buffered SEND/RECV for AMD64 and ARM64 with guarded
fallbacks. Exact snapshots: **554 core/reference + 174 bootstrap files**.
Native availability: **166/173** per architecture; this is partial operand
coverage, not a semantic coverage percentage.

Two fixes returned upstream / Два исправления отправлены обратно:
- [046bb1f](https://github.com/bhgv/Inferno-OS-bhgv/commit/046bb1f7e90619e421a7c23e95025500d343521c): readiness checks no longer write fixture bytecode caches.
- [6945acaf](https://github.com/bhgv/Inferno-OS-bhgv/commit/6945acafa0df51c7ee3e591b1665ddb40d9a27bb): queue tests extract the real channel span/peer guards.

Compiler 0.27 int arithmetic remains FlashVM-only, including table-profile
extensions. Расширения языка в Inferno не переносятся.
[EN](../fir/v1/doc/INT_ARITHMETIC.md) / [RU](../fir/v1/doc/INT_ARITHMETIC.ru.md).

---

# Текущая синхронизация — v0.1643

Experiment остаётся dd0f5d6440de8364a182c8c8eca0976d63e7d485. Проверены обе
стороны; новых входящих коммитов после прошлой серии нет, прежние обратные
исправления находятся в истории. Снимки 545 core/reference- и 174 bootstrap-
файлов точны. Нового общего native-дефекта для обратного патча не найдено;
нативный исходник и модульный ABI не менялись.

Компилятор 0.26 добавляет Unicode len/индексирование/срезы тремя операциями FIR
в конец enum и существующими Dis LENC/INDC/SLICEC. Строковые длины/индексы/символы
имеют тип int; длины таблиц/ключей остаются big. Проверка UTF-8 теперь действует
и для живых строковых констант, созданных через C99 IR API. Эти изменения
frontend/FIR остаются во FlashVM.
[Руководство и пример](../fir/v1/doc/UNICODE_STRINGS.ru.md).
Доступность native 164/173 сохранена, покрытие операндов частичное. Проверяется
сгенерированный Dis на host, а не аппаратный ARM64/полная загрузка Inferno.

---

# Текущая синхронизация — v0.1642

Experiment dd0f5d6440de8364a182c8c8eca0976d63e7d485 перенесён точно:
545 core/reference-файлов и 174 bootstrap. Входящий preflight mapped-копий VM/
каналов проверяет слоты, полные метаданные/карты и payload до изменения ссылок
и публикации. Пустые записи сохраняют nonaccess; AMD64/ARM64 сохраняют pointer-
free native-диапазон 0..256 и C fallback.
[Контракт Inferno](upstream/libinterp/tests/README.mapped-preflight.md).
Два новых native/emulation-драйвера включены в CTest. Прежние обратные исправления
сохраняются в experiment; нового общего native-дефекта при импорте не найдено.

Составные гибридные обновления компилятора 0.25 остаются во FlashVM. Новых ID/
типов FIR, native opcode, helper и контейнерных изменений для них нет. Место и
старое значение захватываются один раз до RHS.
[Руководство](../fir/v1/doc/COMPOUND_UPDATES.ru.md).
SDK теперь заново собирает in-process C99 source+FIR-клиент и проверяет через него
все ветвящиеся учебные исходники без подпроцесса компилятора.
Доступность native остаётся 164/173 с частичным покрытием операндов; эмуляция
не подтверждает аппаратное исполнение или полную загрузку Inferno.

---

# Текущая синхронизация — v0.1641

Experiment проверен в обе стороны на b10f81fa764403adc1321a77136ecfe06e07cb8a.
Новых входящих коммитов после предыдущей серии нет. Снимки 541 core/reference-
файла и 174 bootstrap-файлов остаются точными. Прежние обратные исправления
переходов/JMP уже находятся в истории experiment; нового общего native-дефекта
в этой серии не подтверждено. Дублирующий коммит в Inferno не создаётся.

Компилятор 0.24 добавляет ленивую логику: три операции FIR и скалярную bool-ячейку
кадра в конец enum. Dis использует существующий MOVW; новых opcode VM, helper,
GC-указателей и ABI между кучами нет. Расширения языка/FIR остаются во FlashVM.
[Контракт и учебный пример](../fir/v1/doc/SHORT_CIRCUIT.ru.md).
Доступность native AMD64/ARM64 остаётся 164/173 с частичным покрытием операндов.
Здесь проверяется сгенерированный Dis на host, а не аппаратный порт ARM64
или полная загрузка Inferno.

---

# Текущая синхронизация — v0.1640

Experiment `b10f81fa764403adc1321a77136ecfe06e07cb8a`: 541 точный core/reference-файл и 174 файла bootstrap.
Входящий b10f81f защищает INDB/INDW/INDL/INDF/INDX/LENA и объединяет qword/byte
ядра копирования MOVM/HEADM/MOVMP/HEADMP для AMD64/ARM64. Проверки descriptor/map,
pointer-free native-диапазон 0..256 и C fallback больших записей сохранены.
[Контракт Inferno](upstream/libinterp/tests/README.array-slots-copy-kernel.md).
Три новых native/emulation-драйвера включены в CTest. Прежние обратные исправления
переходов/JMP уже находятся в истории experiment. Нового подтверждённого общего
native-дефекта для обратного коммита при этой синхронизации не найдено.

Сравнения-значения и ! компилятора 0.23 остаются во FlashVM. Приоритеты сверены
с limbo/limbo.y Inferno; новых opcode, helper ABI и тегов контейнера нет.
[Руководство](../fir/v1/doc/COMPARISON_VALUES.ru.md).
Доступность AMD64/ARM64 остаётся 164/173 с частичным покрытием операндов.
Эмуляция ARM64 не подтверждает аппаратное исполнение/полную загрузку Inferno.

---

# Текущая синхронизация — v0.1639

Из experiment `80f859e` перенесены точные проверки операндов/кадров fixed-команд
и восьмибайтные ядра MOVM/HEADM для AMD64/ARM64. Native-диапазон копирования
сохранён 0..256; большие записи исполняются в C. Направление блоков и хвосты
0..7 байт сохраняют перекрытие; пустые payload не читаются. Три новых C/native-
emulation-драйвера зарегистрированы в CTest.
[Контракт Inferno](upstream/libinterp/tests/README.fixed-slots-copy-chunks.md).

Нового подтверждённого дефекта общего native-кода при импорте не найдено.
Прежнее обратное исправление переходов/JMP уже находится в experiment; повторный
коммит не создавался. Расширения лексических ветвей компилятора 0.22 остаются
только во FlashVM. [Руководство](../fir/v1/doc/TERMINAL_ARMS.ru.md).

Доступность AMD64/ARM64 остаётся 164/173, покрытие операндов частичное. Эмуляция
ARM64 не подтверждает аппаратное исполнение и полную загрузку Inferno.

---

# Текущая синхронизация — v0.1638

Inferno experiment: `02b7b7f96183d30f6c75b7415197ecc986e5d411`. 531 файла
ядра/эталонов и 174 bootstrap. Обратное исправление целочисленных переходов/JMP
опубликовано и импортировано побайтно: проверки источников и используемой цели,
отсутствие обращений к ложной цели/источникам JMP, вычисление ARM64-перехода,
сохранение PC/FP/памяти при отказе. [Контракт](upstream/libinterp/tests/README.branch-slot.md).

countof/in/delete, новые операции FIR и hosted-помощники Bigtable остаются
расширениями FlashVM и не отправлялись в Inferno. [Компилятор](../fir/v1/doc/TABLE_MUTATION.ru.md).
Настоящая точка входа ide.dis исполняет Limbo-политику на нашем DisVM и рисует
через FlashVM по типизированным каналам. [Запуск IDE](../ide/dis/app/README.ru.md).

Доступность AMD64/ARM64 — 164/173, покрытие операндов частичное. ISA, контейнеры
и WORD ABI не менялись. ARM64 проверяется эмуляцией; физическая машина/полная
загрузка Inferno — отдельные этапы.

Входящий e14f034 добавляет независимые C/RX и JIT-проверки переходов.
02b7b7f защищает девять числовых команд и расширяет CVTFX на все нормальные
точные степени двойки. Четыре новых native/emulation-драйвера включены в CTest.

---

# Текущая синхронизация — v0.1637

Inferno experiment: `14c3d2c2736936eade4eadfa25e87c8ce5e404ec`. 519 файлов ядра/эталонов,
174 bootstrap. Входящие 7254e9e и 2190eea защищают 18 путей переносов/преобразований
и 24 целочисленных ALU/делительных пути. Их производственные реализации сохранены.
Наше независимо подготовленное деление не публиковалось поверх более широкого
параллельного 2190eea.

Обратный 14c3d2c добавляет шесть real-переходов и независимые проверки 12 команд
деления/real. Ложный переход не трогает цель, истинный проверяет её до PC.
ARM64 вычисляет переход через всю защищённую ветвь. В этом же коммите сохранены
transfercheck и integercheck при встраивании numeric-оракулов в WORD-оснастки.
[Контракт](upstream/libinterp/tests/README.arithmetic-slot.md) ·
[Переносы](upstream/libinterp/tests/README.transfer-slot-preflight.md) ·
[Целочисленная ALU](upstream/libinterp/tests/README.integer-slot-preflight.md).

В CMake — 190 тестов. [Комментированный пример Limbo](../fir/v1/examples/tables/division.b)
покрывает MIN/-1, знак остатка и невыбранное деление на ноль. SDK проверяет его
свежими C99-клиентами до/после оптимизации, в эталоне и обоих режимах VM.

Доступность AMD64/ARM64 — 164/173, покрытие операндов частичное. ABI и ёмкость
эмиттеров не менялись. Арифметика адреса не доказывает принадлежность выделенной
памяти. ARM64 проверяется эмуляцией команд; расширения языка остаются в FlashVM.

---

# Текущая синхронизация — v0.1636

Inferno experiment: `d899383090f6013e8be41da7e3e2c09888856b0b`. 507 файлов ядра/эталонов,
174 bootstrap. При начальной сверке новых входящих коммитов после 9de5fe9 не
было. Наше подтверждённое исправление восьми целочисленных сдвигов возвращено
в Inferno; те же исходники импортированы побайтно в flashVM.

SHLB/SHRB/SHLW/SHRW/LSRW/SHLL/SHRL/LSRL проверяют слот счётчика до чтения WORD.
Недопустимый счётчик сохраняет приоритет ошибки. При допустимом счётчике
проверяются слоты значения и результата до чтения значения. C выдаёт bounds,
JIT AMD64/ARM64 отказывает без записи. Разрядность нативного адреса отделена от
WORD32/WORD64. [Контракт](upstream/libinterp/tests/README.shift-slot.md).

Добавлены в набор CMake независимые проверки: 2 944 случая C и 51 200 случаев
эмуляции команд. Все 24 проверки на предыдущих версиях воспроизводят ошибку
чтения слота счётчика. Существующие проверки арифметики и перекрытий сохранены;
минимальные объявления старых оснасток дополнены bounds-ошибкой.

Доступность AMD64/ARM64 — 164/173 команды, покрытие операндов частичное.
Проверка арифметики адреса не доказывает принадлежность выделенной памяти.
ARM64 проверен эмуляцией команд, физический ARM64 и загрузка ОС — отдельные
этапы. Расширения языка не переносились, ABI WORD не менялся.

---

# Текущая синхронизация — v0.1635

Inferno experiment: `9de5fe916f66efbafa91a7661a82b0b67f2272d3`. 503 файлов ядра
и эталонов, 174 файла bootstrap. Входящие 808a4f6 и 70611aa исправляют проверки
строковых переходов и владельцев в C/AMD64/ARM64. Ложный переход не читает цель;
ошибка времени жизни источника предшествует чтению прежнего владельца назначения
и изменению ссылок/GC. SELF не обращается к неиспользуемому слоту источника.

Обратный коммит ca53cc6 регистрирует отдельный запуск owner-preflight под Python
`-O`: обе архитектуры и все семейства проверок, независимо от интерактивных
фильтров окружения. В CMake flashVM добавлены полный owner-JIT и этот checkpoint;
раньше полный upstream-тест отсутствовал в нашем CMake-наборе.
[Владение](upstream/libinterp/tests/README.owner-preflight.md) ·
[Строковые переходы](upstream/libinterp/tests/README.string-branch-preflight.md).

Доступность AMD64/ARM64 остаётся 164/173 команд. Проверка арифметики адреса не
доказывает принадлежность выделенной памяти. ARM64 проверяется эмуляцией команд;
запуск на физическом ARM64 и полная загрузка Inferno отдельно не подтверждены.
Расширения table/any/int не переносились в Inferno, ABI WORD не изменён.

Финальный входящий 85cd4e5 добавляет проверки слотов для
HEADB/HEADW/HEADL/HEADF/LENL/HEADM/MOVM/TCMP: нулевое копирование не читает
данные, порядок ошибок сохранён, отказ JIT не изменяет память. Оба новых
checkpoint зарегистрированы в нашем CMake. В Inferno также явно проверен
наш ранее возвращённый optimized owner checkpoint.
[Контракт](upstream/libinterp/tests/README.list-slot-preflight.md).

Второй обратный коммит 9de5fe9 исправляет устаревший лимит 64 в тесте raw-copy
после роста защищённого ARM64 HEADM. Используется существующая ёмкость 128,
проверяются все недостаточные размеры, точный результат и защитная ячейка;
очистка кэша покрывает весь код. Production-ёмкость и ABI не менялись.
Все 411 264 случая копирования прошли с Python -O и UBSan на AMD64/ARM64/RV64.

---

# Текущая синхронизация — v0.1634

Experiment: `cd402a0ac5a8a2a8b18d3d60d6229e4bc2788a26`. 496 точных файлов ядра/проверок и 174 файла bootstrap.
Перенесены SLICEC/LENC preflight C/AMD64/ARM64: слоты, нужные заголовки и обратные
ref-ячейки проверяются до доступа. Неверный end отклоняется до чтения владельца;
zero-start префиксы сохраняют работу только с metadata. Native-отказ не пишет
состояние. [Контракт](upstream/libinterp/tests/README.slice-length-preflight.md).

Входящий коммит явно включает наши прежние обратные 9f00116 и 826dae5; независимые
INDC/capacity тесты сохранены. Новых upstream-дефектов для обратной публикации не
подтверждено. Наши int/big-shift расширения не переносятся в Inferno; native WORD
ABI не меняется. AMD64/ARM64 остаются на 164/173 защищённых opcode-путях, без
заявления полного покрытия операндов или физического ARM64/загрузки Inferno.
Ниже — история.

---

# Текущая синхронизация — v0.1633

Experiment: `826dae5c387528f3120728f0bf61204b76639658`. 495 точных файлов ядра/проверок и 174 файла bootstrap.
Входящие исправления ADDC, decimal и INDC/INSC проверяют слоты, String-header и
обратные ref-ячейки до чтений/публикации в C/AMD64/ARM64. Native-отказ не пишет
состояние; доступность остаётся 164/173 opcodes, покрытие операндов частичное.

Обратная публикация — 9f00116: наши независимые C/native регрессии INDC,
регистрация checkpoint и расширенная проверка private capacity. Пока проверялся
ожидаемый head, в upstream появилась более широкая реализация b6c160b для
INDC/INSC. Она сохранена; ранние локальные функциональные коммиты не публиковались.
Возвращённые тесты проходят на b6c160b.
[Независимые проверки](upstream/libinterp/tests/README.indc-preflight.md) ·
[Основное исправление](upstream/libinterp/tests/README.string-cell-preflight.md) ·
[Decimal](upstream/libinterp/tests/README.decimal-preflight.md) ·
[ADDC](upstream/libinterp/tests/README.append-preflight.md).

Наш ациклический CFG таблиц остаётся локальной возможностью компилятора/runtime;
расширение языка в Inferno не переносится. Native AMD64 RX и эмуляция ARM64
не означают испытаний физического ARM64/полной загрузки Inferno. Вторая обратная публикация, 826dae5, исправляет устаревший 64-unit буфер
соседнего decimal-span теста и проверяет sizing/все недостаточные ёмкости
против существующего production-лимита 128. Ниже — история.

---

# Текущая синхронизация — v0.1632

Experiment: `583c3ce6ea8629aa37a1e0237a2b8d99f4a39004`. 488 точных файлов ядра/проверок и 174 файла bootstrap.
CASEC AMD64/ARM64 принимает защищённое совпадение владельца нижней метки или
равные пустые строки в первой середине binary search для таблиц до 4095 строк.
Другие сравнения сохраняют C fallback. Доступность — прежние 164/173 opcodes.

Настоящие C string comparison/duplication проверяют переполнение String-header,
underflow владельца и полный диапазон ASCII/Rune-источника до чтений полей,
allocation и копирования. Новый string-header/source тест выбран и зарегистрирован
вместе с проверками native RX и эмуляции.
[Контракт](upstream/libinterp/tests/README.string-case-native.md).

Полное ядро: 176/176; runtime/IDE: 117/117. Native AMD64 RX и эмуляция ARM64
не означают испытаний на физическом ARM64 или полной загрузки Inferno.
Новых upstream-дефектов для обратной публикации не найдено. Наше расширение
сравнений/решений big остаётся вне Inferno. Ниже — история синхронизаций.

---

# Текущая синхронизация — v0.1631

Experiment: `a3fb27ec812fd0c7d544451e2a003d6a4bf84034`. 486 точных файлов ядра/проверок и 174 файла bootstrap.
CASEC AMD64/ARM64 проверяет полную declared-cell область счётчика до чтения,
а непустые узкие cell4-таблицы отклоняет до чтения строк и default PC.
Однострочный путь также принимает разные владельцы пустых строк и пары H/empty,
проверяя длину и ёмкость. Доступность остаётся 164/173 активных opcodes:
расширены защищённые входы, не реализован полный native-поиск строк.

Bootstrap сохраняет padding big case-таблиц, NaN payload и quoted-байты данных/
исходных путей. Обе реализации Str получили исправления radix/digit и десятичного
накопления экспоненты. Новые тесты исполняют настоящий C-сериализатор и общие
C/Limbo integer-тела Str; это не полный ассемблер и не self-hosted запуск Limbo.
[Assembly](upstream/limbo/tests/README.asm-roundtrip.md) ·
[Str](upstream/appl/lib/tests/README.string-radix-host.md).

У всех 175 тестов ядра есть успешные результаты. Начальный сбой assembly-теста
был вызван невыбранной зависимостью g_fmt.c: она добавлена в точный снимок,
повторная проверка проходит. Native AMD64 RX и эмуляция инструкций ARM64 не
означают испытаний на физическом ARM64. Новых upstream-дефектов для обратной
публикации не найдено. Наши big CFG и verifier относятся к локальному компилятору;
расширение таблиц Limbo в Inferno не переносится.

---

# Текущая синхронизация — v0.1630

Experiment: `1ae1470f6960173a2330f212ee76d32843ae2736`. Снимки: 478 точных файлов ядра/регрессий и 174 bootstrap.
Перенесён защищённый ICASEC для AMD64/ARM64: пустая таблица либо точное
совпадение владельца нижней границы в однострочной таблице. Доступность стала
164/173 активных операций; это ограниченный нативный путь, а не полный поиск
по строковым таблицам. Остальные случаи сохраняют C-путь и его ошибки.

Полная ширина счётчиков, заголовки String, границы PC и дальние переходы к отказу
проверяются до записи REG.PC. Нет выделения памяти, runtime callback, изменений
GC/счётчиков ссылок или FP-состояния. [Контракт и проверки](upstream/libinterp/tests/README.string-case-native.md).

Полный набор ядра: 173/173. Нативный AMD64 RX и эмуляция ARM64 — разное
свидетельство; физический ARM64 этим не подтверждается. Нового обратного
исправления в этой серии не найдено. Расширение языка в Inferno не переносится.
Ниже сохранена история.

---

# Текущая синхронизация — v0.1629

Experiment: `5c79135244b0c2e651d200ec3caa4b589833dc60`. 474 точных файла ядра/регрессий и 174 bootstrap.
Перенесены raw/mapped каналы NEWCM/NEWCMP и pointer-free mapped-списки CONSMP
для AMD64/ARM64. Доступность нативных путей — 163/173 активных команд; проверки
могут оставить конкретный случай интерпретатору. Физический ARM64 не проверен.

Перенесены защищённые строковые case-таблицы, откат частичных удержаний mapped
головы списка и согласованные с IBY2WD числовые case/goto-таблицы. Ширина
значащих полей, размер ячейки, VM WORD и нативного указателя различаются явно.
Пять контрольных падений старой Python ABI-фикстуры ушли после upstream b579b5f.
Наш обратный коммит 5c79135 разворачивает список полей и поясняет ABI-оракул;
функциональное обновление ABI уже пришло в b579b5f и не перезаписывалось.

[Ячейки PC-таблиц](upstream/libinterp/tests/README.pc-table-cells.md) ·
[Mapped-списки](upstream/libinterp/tests/README.mapped-cons-native.md) ·
[Каналы](upstream/libinterp/tests/README.payload-channel-native.md).
Полный набор ядра прошёл 171/171. Расширение таблиц Limbo в Inferno не переносилось.
Ниже сохранена история.

---

# Текущая синхронизация — v0.1626

Приоритет: x86-64 и ARM64. Experiment: `875d12263efa72bab84d27f0d4215713e7da9465`.
Перенесены пять новых JIT-конструкторов скалярных каналов NEWCB/NEWCW/NEWCL/NEWCF/NEWCP
из cc51c63. Доступность AMD64/ARM64 — 160 из 173 активных операций; это наличие
защищённого нативного пути, а не покрытие всех входных данных. RV64 не менялся.

Найдено и возвращено в Inferno наше исправление TCMP: малые ненулевые адреса
владельцев больше не вызывают переполнение при вычитании расстояния до Heap.t
или чтение по адресу ноль. Интерпретатор и оба приоритетных JIT проверяют границу
перед чтением Type. На прежнем C-коде проблема воспроизведена UBSan. Отказ
нативного пути сохраняет состояние VM; GC и счётчики ссылок не меняются.

[Каналы](upstream/libinterp/tests/README.scalar-channel-native.md) ·
[TCMP: контракт и границы](upstream/libinterp/tests/README.typecheck-prefix.md).
Снимок содержит 453 точных исходных файла; bootstrap — 174. Расширение table
в Inferno не переносилось. Реальный ARM64 запуск остаётся отдельной проверкой.
Ниже сохранена история синхронизации.

---

# Текущая синхронизация — v0.1625

Зафиксирован experiment: `b2da16226dfc5bc0e0c754ae5d95fcde6cc1afb9` (три коммита после b9a569e).
Снимок Dis: 446 исходных файлов без изменения байтов; временный bootstrap: 174.

Перенесены нативные ICONSM/ICONSP для AMD64/ARM64: создание списков с сырыми
данными и указателями, отложенная публикация результата и актуальные GC/refcount
после выделения памяти. Для копий свыше 256 байт сохраняется проверяемый путь
интерпретатора. Доступность RV64 не меняется. Проверки префикса кучи теперь
отклоняют и точную границу, которая приводила бы к обращению по адресу ноль.
[Контракты владения и проверки](upstream/libinterp/tests/README.payload-cons-native.md).

В компиляторе перенесены ограничения роста истории исходников и поиск без
переполнения середины диапазона. Подключены исходные регрессии выбора fixed
операций и свёртки преобразований/строк. Это исправления опорного компилятора
и bootstrap; наш Coco/R-фронтенд и его табличный Dis-бэкенд остаются отдельными.

Добавлены семь тестовых драйверов: раскладка payload, нативный AMD64, эмуляция
AMD64/ARM64, история исходников, fixed, преобразования и строки. Модели WORD32/64
и эмуляция ARM64 не заменяют запуск на физическом ARM64. Расширение table в
Inferno не переносилось. Ниже сохранены исторические отчёты синхронизации.

---

# Current synchronization / Текущая синхронизация — v0.1593

Selected experiment: `74c193724a7352b8fda2724a5cf10029885be68c`.
Includes unique array-view SLICEA leaves and self-load ownership from 4d9ce2e,
plus our returned Loader construction/scalar result-slot fix.
[Module audit](libraries/MODULE_AUDIT.md) · [Upstream Loader contract](upstream/libinterp/tests/README.loader-results.md).
Vendored files remain byte-identical; native ARM64/RISC-V platform execution
is not established by host/emulator fixtures. Earlier reports follow as history.

---

# Обновление Dis, JIT и Sys — v0.1540

[English](SYNC.md) · [Состав снимка](UPSTREAM.json) · [Как обновлять](../tools/UPSTREAMS.ru.md)

Источник: https://github.com/bhgv/Inferno-OS-bhgv, ветка **experiment**.
Новый зафиксированный коммит: `f948343b711c8300dbf76417254c18a7a8d2f5ff`.
Предыдущий снимок Dis: `24892373ee09a2a23dc39b9dedc01fb7af61d8c1`.
Между ними восемь коммитов. Сборка не обращается к плавающему HEAD.

## Перенесено

- AMD64/ARM64: дополнительные вещественные операции, степени, преобразования,
  округление и знаковые фиксированные коэффициенты; деление/остаток на −1.
- Sys в emu/os: публикация FD, владение старыми/новыми результатами, очистка
  при отказах, невыровненные результаты open/create/fildes/pipe и Connection.
- Диагностика процессов и запросы к куче: ширина VM-слов и границы результатов.
- Bootstrap Limbo: фиксированная арифметика сохраняет вычисление делителя
  и исключение деления на ноль, когда масштабирование сводит результат к нулю.

В disVM находятся **192** отобранных файла; в tools/limbo-bootstrap — **174**.
Снимки побайтово совпадают с исходным коммитом; лицензии и SHA-256 сохранены.
Новые devprog.c используются только для извлечения проверяемых функций в
регрессиях, а не как дополнительная служба ОС. limbo/gen.c и types.c в снимке
Dis — входы регрессии compiler/VM; отдельный компилятор из них там не собирается.
Math/Geometry, Draw/Tk/Prefab и остальная Inferno в этот перенос не добавлялись.
Собственный Coco/R-компилятор остаётся приоритетным; bootstrap временный.

## Адаптация сборки

DisVM CMake-компонент обновлён до 0.1.4; минимальная CMake остаётся 3.10.
Добавлены восемь регрессий Sys/диагностики, проверка фиксированного деления
на ноль и две native AMD64-проверки вещественной арифметики. Ещё восемь
проверок собирают реальные ARM64/RV64-эмиттеры для моделей WORD32/WORD64,
включая RV64I/Zmmul/M. Эти восемь проверок не исполняют ARM64/RV64 код.
Генерация TLS-представления R, безопасные точки GC, загрузчик и канальный мост
не потребовали изменения ABI; их проверки проходят с новым снимком.
module/sys.m не изменился: обновлена реализация, а не интерфейс заголовка.

## Проверено на Linux AMD64, GCC, CMake 3.10.2

- Обычная сборка ядра и исходный набор с новыми регрессиями: **42/42**.
- TLS-регистры, настоящий мост FlashVM, GC workers и восемь дополнительных
  проверок эмиттеров: **62/62**.
- Восемь регрессий Sys/диагностики отдельно с UBSan: **8/8**.
- Bootstrap собран из исходников, продуктовый smoke-test: **1/1**.
- Шесть модулей IDE пересобраны этим bootstrap с шестью файлами .sbl.
- Оба снимка прошли проверку происхождения и содержимого манифестов.

Sys-регрессии исполняют исходные тела функций с контролируемыми границами
ОС/планировщика. Это не запуск полного Sys-модуля на собственном Dis-хосте.
Полноценный исполняемый Dis-хост, регистрация модулей, VFS/Sys-привязка и
запуск IDE остаются незавершёнными. Новое обновление их не подменяет тестовыми
успехами. Windows/macOS/Android и физические ARM64/RV64 здесь не запускались.
Подтверждённых новых ошибок upstream этим проходом не обнаружено.

## Повторить

```sh
python3 tools/vendor-sync.py disVM
python3 tools/vendor-sync.py tools/limbo-bootstrap
./build-workspace.sh --projects limbo-bootstrap,disvm,ide --tests
```

Для расширенной проверки моста передайте CMake явные DVM_NATIVE_REGISTERS,
DVM_FLASHVM_SOURCE_ROOT и DVM_FLASHVM_LIBRARIES согласно [README](README.ru.md).
Все runtime-зависимости и границы владения остаются явными.
