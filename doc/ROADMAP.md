# План завершения FlashVM / FlashVM completion roadmap

Принят пользователем 12 октября 2026 года. Основной ориентир дальнейшей
разработки. Исходная контрольная точка: v0.14. Добавление этого документа в
v0.15 не означает появления новых возможностей VM.

Цель: полная реализация Flash SWF ≤ 8 и соответствующих AS1/AS2 API,
запрошенные расширения, JIT и поддержка Windows/Linux/macOS/Android.
Требования: C89/C++98, без STL-контейнеров; платформенные адаптеры в ports/;
внешние библиотеки в external/, статическое подключение; читаемый код и
комментарии, тесты, документация на русском и английском.

## Просмотрщик документации (реализован в v0.544)

Пользователь 2026-10-17 запросил отдельный браузерный просмотрщик после текущих
работ. Запускаемый Python-файл разместить в корне репозитория; остальные файлы
движка — в `doku_viewer/`. Поддерживать Python 2.7.9 и Python 3: локальный HTTP
сервер, рендер Markdown и навигация по корневым README, основной документации,
каталогу примеров и README каждого примера. Сохранять относительные ссылки,
якоря и переходы назад. Ссылки на исходники открывать внутри просмотрщика с
подсветкой синтаксиса (например, Pygments), нумерацией строк и прокруткой к
нужной строке. Версии зависимостей для Python 2/3 выбирать явно; проверить обе
ветки интерпретатора отдельно, не выдавая запуск на Python 3 за проверку 2.7.9.
При реализации документировать установку, запуск и ограничения; сервер по
умолчанию слушает loopback, доступ к файлам ограничен деревом репозитория.

Deferred documentation viewer: root Python launcher, engine in `doku_viewer/`,
Python 2.7.9/3 compatibility, browser Markdown navigation across README/docs/
examples, and syntax-highlighted source links with exact line navigation.
Implemented in v0.544: see [viewer guide](../doku_viewer/README.md).
Implementation does not close the remaining roadmap stages.

## Этапы

| № | Этап | Оставшаяся работа |
| --- | --- | --- |
| 1 | Простые типы и стандартные объекты | Полноценные UTF-16 строки, встроенный NUL, одиночные суррогаты, Unicode-регистр; оставшаяся семантика Object/Array, наследуемые индексы, заимствованные методы, registerClass. |
| 2 | Интерпретатор AVM1 | DefineFunction2, регистры и preloads, связанность arguments с параметрами, области видимости, интерфейсы, исключения и точные правила преобразований для разных версий SWF. |
| 3 | MovieClip и сцена SWF | Связать модель MovieClip с ресурсами и display list SWF; attach/duplicate, глубины, кадры, маски, размеры, shape hit testing, drag/input, загрузка клипов и создание текстовых полей. |
| 4 | Графика, текст и мультимедиа | Градиенты, bitmap-заливки, фильтры и blend modes; полное поведение TextField/TextFormat, встроенные и системные шрифты; звук, потоковое аудио и видео соответствующих версий Flash. |
| 5 | Остальные стандартные API | Полный перечень объектов и методов AS1/AS2 и реализация недостающих: загрузчики, XML, сетевые объекты, SharedObject, события, ввод и системные службы. |
| 6 | Расширения FlashVM | Согласованная модель потоков, каналов, мутексов, метатаблиц и сериализации всех объектов, включая встроенные; перенос ресурсных аннотаций; сетевые обрывы, устаревшие ссылки и распределённые блокировки; API C-расширений по образцу Lua. |
| 7 | GC и отладчик | Подтвердить полноту корней AVM1, нативных функций и новых подсистем, финализацию и циклы при конкуренции; дедлоки с привязкой к исходникам, перехват и заморозка посылок каналов для анализа. |
| 8 | JIT | Генерация кода ARM, ARM64, x86, x64, RISC-V и RISC-V64; согласование с GC, исключениями, отладчиком и интерпретатором; проверка эквивалентности исполнения. |
| 9 | Переносимость и выпуск | Реальные сборки и запуски на Windows, Linux, macOS и Android; статические зависимости, выбранные графические адаптеры, документация RU/EN и воспроизводимые архивы с полной историей Git. |

Все этапы остаются открытыми; реализованные части не означают завершения этапа.
Конкретные подтверждённые возможности исходной точки и ограничения описаны в
[STANDARD_OBJECTS.ru.md](STANDARD_OBJECTS.ru.md), [PROPERTY_WATCH.md](PROPERTY_WATCH.md)
и [OBJECTS_CHECKPOINT.md](OBJECTS_CHECKPOINT.md).

## Порядок работы и контроль выполнения

Ближайший порядок: этап 1 → этап 2 → этап 3 → этапы 4 и 5.
Этапы 6 и 7 сопровождают разработку каждого объекта и подсистемы; их проверки
нельзя откладывать до конца. Этап 8 требует устойчивой семантики интерпретатора.
Платформенные проверки этапа 9 наращиваются по мере реализации, затем проходят
общую приёмку.

Перед продолжением разработки читать этот план и выбирать следующий конкретный
пункт. Вести реестр совместимости SWF/AS с отметками «отсутствует», «реализовано»,
«проверено», «завершено». Для проверенных пунктов указывать реальные тесты,
конфигурации и оставшиеся ограничения. Пересматривать реестр при изменениях.
Закрывать этап только после подтверждающих тестов и обновления RU/EN документации.
Не оценивать процент готовности или срок без полного реестра и основания.

Каждый коммит: повысить версию, использовать английское сообщение с префиксом
v<версия>, провести необходимые проверки. Внутренний Git основной. По действующей
команде пользователя обновлять зеркало после каждого коммита без force push,
с HTTP-буфером 67108864; после fetch проверять серверный коммит. Это правило
действует до нового указания пользователя.

## Итоговая приёмка

- Реальные SWF версий до 8 включительно и сравнение поведения с эталоном;
  полный реестр объектов, методов, Action-кодов и поддерживаемых SWF-тегов.
- Сериализация и десериализация всех заявленных типов через очередь, файл и сеть;
  циклы, общие ссылки, ресурсы и метатаблицы; исключения с кодом, текстом и контекстом.
- Повреждённые входные данные, отказы выделения памяти, финализация, корни GC;
  проверки памяти с явно указанными ограничениями инструментов.
- Длительные многопоточные и сетевые испытания, обрывы соединений и дедлоки;
  работоспособность средств отладки.
- Совпадение результатов интерпретатора и каждого заявленного JIT backend.
- Сборка и запуск на каждой целевой платформе и проверка поддерживаемых адаптеров.
- Документация и примеры RU/EN; исходники с полной историей Git в архиве с версией.
  Технические результаты выдаются отдельным архивом и не коммитятся.

Уточнение выдачи от пользователя 15 октября 2026 года: продолжать в существующей
рабочей копии и обновлять только зеркало. Не создавать и не выдавать архивы
исходников/локального Git до новой явной просьбы. Это заменяет прежнюю выдачу
архивов; проверки воспроизводимости и сохранение истории в зеркале остаются.

Количество прошедших тестов само по себе не доказывает полноту совместимости.
Особенно велики оставшиеся объёмы MovieClip/SWF, мультимедиа и JIT.

## English reference

This is the user-approved completion roadmap, adopted on 12 October 2026.
The implementation baseline is v0.14; v0.15 records the plan without adding VM
features. All nine stages remain open:

1. Complete primitive types and Object/Array semantics, including full UTF-16.
2. Complete AVM1 execution, Function2, registers, arguments and version semantics.
3. Integrate MovieClip with the full SWF display list and resource model.
4. Complete graphics, text, audio and video for the supported Flash versions.
5. Inventory and complete the remaining standard AS1/AS2 objects and APIs.
6. Complete threads, channels, mutexes, metatables, graph transport and Lua-style
   C extensions, including remote failures and shared resource identity.
7. Verify GC roots/cycles/finalization throughout, and complete debugger support
   for source locations, deadlocks and intercepted/frozen channel payloads.
8. Implement and verify ARM/ARM64/x86/x64/RISC-V/RISC-V64 JIT backends.
9. Verify real Windows/Linux/macOS/Android builds and deliver documented releases.

Work through stages 1, 2 and 3 first, then 4 and 5. Stages 6 and 7 accompany all
implementation work. JIT depends on stable interpreter semantics. Platform
verification grows throughout and is part of final acceptance.

Read this roadmap before resuming development. Maintain a detailed compatibility
inventory with absent/implemented/tested/complete states, test evidence, profiles
and limitations. Do not close a stage without acceptance tests and RU/EN docs.
Do not claim a percentage or deadline without an evidence-based inventory.
Final acceptance requires real SWFs, reference comparisons, malformed-input and
allocation-failure tests, graph transport tests, concurrency/network endurance,
GC/memory checks, JIT equivalence and real runs on every target platform.
Keep C89/C++98 and project-owned containers; static libraries belong in external/,
platform adapters in ports/. Internal Git is authoritative. Increment the version
for each commit and use an English v<version> subject. The current standing user
instruction authorizes a verified mirror update after every commit until revoked.
Ship versioned source archives with full Git history and separate validation
archives. Never commit generated validation logs.

Delivery update, 15 October 2026: the user's current instruction is to continue
in the existing checkout and update the verified mirror only. Do not create or
export source/local-Git archives unless explicitly requested again. This replaces
the earlier archive-delivery requirement, while build reproducibility and full
history preservation in the mirror remain acceptance requirements.

## Контрольная точка v0.346 / Native checkpoint

По указанию пользователя 15 октября приоритет текущей работы перенесён на
работающий JIT и возможность запускать интерпретатор. Добавлен явный Linux
x86-64 backend для ограниченного подмножества AVM1 с примером запуска; см. JIT.md.
Это начало реализации стадии 8, не её закрытие. Предыдущая семантика VM,
полнота инструкций, остальные backend и платформенная приёмка остаются открытыми.

The user reprioritized working JIT/interpreter use on 15 October. v0.346 adds an
explicit Linux x86-64 native subset and runnable C89 example (JIT.md). Stage 8
remains open: this is not full AVM1 coverage, automatic compilation, other-CPU
code generation or cross-platform acceptance.

Контрольная точка v0.347: по уточнению пользователя добавлены опции interpreter/JIT
при входе, переключение для следующих кадров во время исполнения, автоматический
контекстный кеш и явный диагностический replay с логом/статистикой. См.
EXECUTION_MODES.md. Активный native-кадр не переписывается; побочные эффекты
повтора не откатываются. Полнота backend/AVM1 и стадии 8/9 остаются открытыми.

v0.347 adds user-requested runtime mode selection/switching and opt-in interpreter
exception replay with logs/statistics. See EXECUTION_MODES.md. Active native
frames are not deoptimized in place; replay is not state rollback. Full native
coverage and roadmap acceptance remain open.

Контрольная точка v0.348: добавлены Linux ABI encoders x86, ARMv7 A32, ARM64,
RV32I и RV64I, а также исключение тел с breakpoint из JIT, примеры workers и
нескольких VM. См. JIT_ARCHITECTURES.md. Новые ISA проверены в Unicorn; полные
сборки/запуски VM на чужих CPU и ОС не проверены. Стадии 8/9 остаются открытыми.

v0.348 adds five Linux machine encoders and interpreter-only breakpoint bodies,
plus worker/multiple-VM examples. New ISA evidence is synthetic-helper emulator
execution, not full foreign-CPU/OS acceptance; stages 8/9 remain open.

Контрольная точка v0.349: повторяемые Release-профили AGG/Cairo/minimal,
проверочный прогон с журналами и статусом ошибок, ADB-адаптер исполнения CTest.
Кросс-сборка больше не регистрирует Python-сценарии, запускающие целевые бинарники
на хосте в обход адаптера. См. PLATFORM_VALIDATION.ru.md. Реальные запуски
Windows/macOS/Android и окончательная приёмка всех стадий по-прежнему необходимы.

v0.349 adds native release-profile validation and an explicit-device Android
CTest adapter. Cross-builds omit host-only process harnesses instead of launching
foreign binaries locally. See PLATFORM_VALIDATION.md. This is stage 9 validation
infrastructure; it does not close any stage or supply missing target-OS evidence.

v0.350 extends the native subset with SetVariable, DefineLocal/DefineLocal2,
GetMember/SetMember and Delete/Delete2 using the shared interpreter C services.
JIT_REFERENCES.md records host/SWF4–8, GC/setter/exception and allocation checks.
Это расширение стадии 8; полнота AVM1 и итоговая приёмка всех стадий ещё открыты.

v0.351 adds native float/double literals and seven unary operations with shared
interpreter numeric decoding, explicit/automatic execution checks and restricted
memory-tool coverage (JIT_NUMBERS.md). Полная семантика AVM1, оставшиеся подсистемы
и реальные запуски других ОС по-прежнему необходимы для закрытия всех стадий.

v0.352 adds native register reads/stores over shared prepared Function2/legacy
banks and rooted native root-block banks. JIT_REGISTERS.md records versioned,
manual/automatic, GC and allocation checks. Это ещё один проверяемый пункт стадии
8; полнота всех стадий, другие CPU/ОС и финальная приёмка остаются открытыми.

v0.353 adds ConstantPool and indexed Push to native execution, with captured
activation snapshots, runtime branch-dependent selection and scoped nested-body
restoration (JIT_POOLS.md). Все форматы Push представлены в нативном подмножестве;
полная реализация инструкций, остальных подсистем и приёмка всех стадий ещё нужны.

## v0.354: native method dispatch

CallMethod/NewMethod use shared rooted AVM1 services, preserving count/name
validation, ordered arguments, nullish-owner behavior and constructor semantics.
JIT_METHODS.md records host/SWF4–8, explicit/automatic, GC, exception and
allocation checks. Именованные вызовы, остальные инструкции и все стадии
роудмапа, включая реальные запуски других платформ, остаются открытыми.

## v0.355: native named calls

CallFunction/NewObject use shared rooted lexical/target lookup and invocation.
JIT_NAMED_CALLS.md records versioned host/SWF4–8 comparisons, local/qualified
function names, literal loaded constructor names, collecting name/count coercion,
exceptions and allocation failures. Автоматический JIT проверен для обоих тел
вложенного вызова. Все стадии и реальные запуски других платформ ещё открыты.

## v0.356: native target selection

SetTarget/SetTarget2 use shared versioned selection and scene validation.
JIT_TARGETS.md records host/SWF4–8 comparisons, literal/dynamic/missing/removed
paths, collecting path coercion, exceptions, compile/invoke allocation failures
and suspended-target restoration. Все стадии и приёмка других CPU/ОС открыты.

## v0.357: native string Actions

Twelve string Actions now share counted/versioned interpreter services, preserving
conversion order, rooted operands and existing SWF4–5 ASCII/character limits.
JIT_STRINGS.md records exact UTF-16, missing-input, manual/automatic, collecting/
throwing coercion, allocation and returned-text lifetime checks. Полная строковая
совместимость Flash, другие инструкции и все стадии приёмки ещё открыты.

## v0.358: native Array/Object initializers

InitArray/InitObject use shared count normalization and construction, preserving
entry order, independent missing pair members, own Array slots and counted keys.
JIT_LITERALS.md records versioned native/interpreted, collecting/throwing callback,
allocation and result-lifetime checks. Определения функций, остальные инструкции
и полная совместимость и приёмка всех стадий ещё открыты.

## v0.359: native legacy comparisons and logic

Equals/Less and And/Or use shared versioned coercion and metamethod services.
JIT_LEGACY.md records 1944 primitive combinations in three execution modes,
missing inputs, numeric conversion order, collecting/throwing left/right hooks
and allocation failures. Остальные инструкции, полная совместимость и все стадии
приёмки, включая реальные запуски других CPU/ОС, остаются открытыми.

## v0.360: native class Actions

Extends/Implements/InstanceOf/Cast share rooted class/interface services.
JIT_CLASSES.md records isolated versioned execution, interface snapshots,
collecting/throwing accessors and primitive boxing, missing operands and allocation
failures. Полная семантика Flash, остальные инструкции и все стадии открыты.

## v0.361: repeatable encoder validation gate

validate_platform.py --jit-emulator resolves the configured Release generator
from CTest, records Unicorn version and runs freshly generated ISA/ABI fixtures.
Failures and optimized Python cannot produce passing emulation evidence; five
host-only orchestration tests cover failure states and stale-fixture isolation.
PLATFORM_VALIDATION.md records usage and scope. Это инфраструктура стадий 8/9,
а не реальный запуск полной VM на других CPU/ОС или завершение стадий.

## v0.362: native display-property and path Actions

GetProperty/SetProperty/TargetPath share rooted scene and counted-path services.
JIT_DISPLAY.md records isolated host/SWF4–8 execution, target selection, ordered
collecting/throwing coercion, skipped readonly/invalid writes, counted native paths
and allocation failures. Остальные поля, полная семантика Flash и все стадии
приёмки, включая реальные запуски других CPU/ОС, остаются открытыми.

## v0.363: native effect Actions

Trace/Throw/GetTime/RandomNumber share rooted execution and exception services.
JIT_EFFECTS.md records host/SWF4–8 equivalence, collecting/throwing conversions,
trace sink failures, exact random state, clock samples and allocation failures.
Полная совместимость Flash, остальные инструкции и реальные запуски на других
ОС остаются открытыми; ни одна стадия не закрыта.

## v0.364: native enumeration Actions

Enumerate/Enumerate2 share rooted qualified/plain lookup and property-name snapshots.
JIT_ENUMERATE.md records host/SWF4–8 equivalence, counted/hidden/inherited keys,
MovieClip lookup, collecting/throwing conversions/getters, allocation failures and
exact stack bounds. Полная совместимость Flash и реальные запуски других ОС/CPU
остаются открытыми; ни одна стадия не закрыта.

## v0.365: native timeline Actions

NextFrame/PreviousFrame/Play/Stop/GotoFrame/GotoLabel/GotoFrame2 share intrinsic
timeline services, rooted request conversion and versioned target selection.
JIT_TIMELINE.md records isolated host/SWF4–8 equivalence, bounds/labels/bias,
collecting/throwing coercion, changed targets, malformed records and compile/invoke
allocation failures. Полная сцена SWF, совместимость Flash и реальные запуски
других ОС/CPU остаются открытыми; ни одна стадия не закрыта.

## v0.366: native clone, remove and drag Actions

CloneSprite/RemoveSprite/StartDrag/EndDrag share rooted native scene services.
JIT_SCENE.md records isolated host/SWF4–8 equivalence, intrinsic clone/removal,
selected-target repair, ordered collecting/throwing coercion, detached drag roots,
EndDrag exception/retry and allocation failures. Полная совместимость сцены Flash,
оставшиеся инструкции и реальные запуски других ОС/CPU остаются открытыми.
Ни одна стадия не закрыта.

## v0.367: native frame waits and counted skips

WaitForFrame/WaitForFrame2 share rooted availability queries and native conditional
branches over complete source records. JIT_WAIT_FRAMES.md records host/SWF4–8
equivalence, strict text/missing inputs, collecting/throwing conversion, compile/
invoke allocation failures, invalid targets and exact early/implicit-End budgets.
Неподдерживаемые блоки, полная совместимость Flash и реальные запуски других
ОС/CPU остаются открытыми; ни одна стадия не закрыта.

## v0.368: native URL and stored-frame dispatch

GetURL/GetURL2/Call share rooted host/frame services. Validated stored frame
blocks now follow automatic JIT selection with named-breakpoint exclusion.
JIT_DISPATCH.md records host/SWF4–8 behavior, counted requests, ordered GC/throws,
nested frame/pool/target preservation, bounded recursion/budget, fallback and
compile/invoke allocation failures. Полная совместимость Flash, browser/streaming
и реальные запуски других ОС/CPU остаются открытыми; ни одна стадия не закрыта.


v0.369: bounded stored-frame diagnostic replay preserves implicit End framing;
`execution_mode_frame_replay` checks host/SWF4–8 and success/throw retries.
Диагностический повтор сохраняет границы кадра с неявным End, исходное исключение,
бюджет и корни. This correction does not close any roadmap stage; все стадии открыты.


v0.370: DefineFunction/DefineFunction2 compile to rooted native creation/publication.
Shared interpreter/native materialization retains executed constant pools, lexical
closures, parameter registers and preload metadata; nested unsupported bodies fall
back independently. JIT_DEFINITIONS.md / .ru.md record tests and remaining gaps.
Нативные определения функций проверяются с GC/setter-исключениями, повреждёнными
записями и отказами памяти. With/Try и полная совместимость остаются открытыми;
ни одна стадия не закрыта.


v0.371: native With owns compiled child bodies and shares rooted operands,
registers, executed pools and instruction accounting. Nested Return propagates;
lexical scopes and code activity unwind on errors/GC/OOM. Aggregate native bytes
include children; 64-level nesting and breakpoint/cache behavior are tested.
JIT_WITH.md / .ru.md record evidence and limits. Try/remaining Actions, full
conformance and real foreign-platform acceptance remain open; все стадии открыты.


v0.372: native Try/Catch/Finally shares compiled block state with With, roots
suspended baselines/returns/exceptions/host bindings, restores rich faults and
preserves versioned catch-name/register rules. Root JIT selection now contains
allocation failure before breakpoint lookup. JIT_TRY.md / .ru.md document tests,
non-script error behavior and remaining conformance/platform gaps. Ни одна стадия
не закрыта; полная совместимость и реальные запуски других ОС остаются открытыми.


v0.373: borrowed loaded negative-length splice uses signed own insertion keys,
captured conversion bounds and rooted empty removals; native negative storage
and signed overflow remain unsupported. Этапы остаются открытыми.
See [EN](ARRAY_NEGATIVE_SPLICE.md) / [RU](ARRAY_NEGATIVE_SPLICE.ru.md).

Validation v0.373: 769 AGG / 769 Cairo / 750 minimal Release checks pass after
private-header rebuild and ELF permission repairs/reruns; five restricted
heap/global ASan+UBSan splice checks pass (stack ASan/LSan disabled).


v0.374: loaded borrowed push/pop/shift/unshift accept negative own lengths;
signed keys, distinct length/deletion rules, flags, budgets and allocation
cleanup are covered. See [EN](ARRAY_NEGATIVE_ENDS.md) / [RU](ARRAY_NEGATIVE_ENDS.ru.md).
Все стадии остаются открытыми; native negative storage/overflow remain open.

Validation v0.374: all 771 AGG / 771 Cairo / 752 minimal Linux Release checks
have passing evidence after ELF execution-permission repairs and failed-test
reruns. Five restricted heap/global ASan+UBSan checks pass (stack ASan and LSan
disabled): both new end-operation tests, legacy push/unshift and negative splice.


v0.375: loaded slice preserves negative signed length through bound conversion
and copies selected own negative keys for borrowed/native sources. Dense holes,
raw accessors, source retention, budget and allocation cleanup are covered.
See [EN](ARRAY_NEGATIVE_SLICE.md) / [RU](ARRAY_NEGATIVE_SLICE.ru.md).
Все стадии остаются открытыми; signed overflow/result Int32 bounds remain.

Validation v0.375: 773 AGG / 773 Cairo / 754 minimal Linux Release checks
have passing evidence (AGG/Cairo ELF permission repairs and failed-test reruns;
minimal passed the full run directly). Five restricted heap/global ASan+UBSan
checks pass: both negative-slice tests, both negative-end tests and legacy slice.
Stack ASan and LeakSanitizer remain disabled in this environment.


v0.376: loaded generic shift/unshift/reverse/splice drop the arbitrary 65536
source-length restriction; charged loops keep shared budget/cancellation policy.
Successful native/borrowed traversal, SWF4–8 budget failure and allocation/GC/retry
are covered; host/sort limits remain. [EN](ARRAY_LOADED_LARGE_MUTATION.md) /
[RU](ARRAY_LOADED_LARGE_MUTATION.ru.md). Все стадии остаются открытыми.

Validation v0.376: all 775 AGG / 775 Cairo / 756 minimal Linux Release checks
have passing evidence after nine ELF permission repairs and failed-test reruns
(4 AGG, 2 Cairo, 3 minimal). Five restricted heap/global ASan+UBSan checks pass,
including both new large-mutation checks, legacy unshift/splice flags and negative
end operations. Stack ASan and LeakSanitizer remain disabled. The new scoped
constant header also passes a strict C89 syntax probe.


v0.377: loaded native/borrowed sort and sortOn drop the positive 65536 bound.
Partial rooted input capture, descending reversal and index output are charged;
large permutation/index sorting and budget/OOM/GC/retry are covered. Host bounds
remain. [EN](ARRAY_LOADED_LARGE_SORT.md) / [RU](ARRAY_LOADED_LARGE_SORT.ru.md).
Все стадии остаются открытыми.

Validation v0.377: all 777 AGG / 777 Cairo / 758 minimal Linux Release checks
have passing evidence after five ELF permission repairs (2/2/1) and six failed-test
reruns, including the dependent timezone profile runner. Six restricted heap/
global ASan+UBSan checks pass: both new large-sort checks, legacy allocation,
comparator, snapshot and no-op checks. Stack ASan and LeakSanitizer remain
disabled. The revised scoped constant header passes strict C89 syntax checking.
Original external SWF traces were not rerun; no new external oracle pass is claimed.


v0.378: SWF4–6 display paths accept non-ASCII names through allocation-free
Flash UTF-16-unit lowercase comparison; SWF7–8/host remain sensitive. Child/alias
lookup, counted NUL/surrogates, collecting script getters, OOM/retry and native/
interpreted target selection are covered. [EN](MOVIECLIP_UNICODE_PATHS.md) /
[RU](MOVIECLIP_UNICODE_PATHS.ru.md). Все стадии остаются открытыми.

Validation v0.378: all 781 AGG / 781 Cairo / 762 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and failed-test
reruns (two per profile). Seven restricted heap/global ASan+UBSan checks pass:
four new Unicode path/target checks plus existing resolver, JIT target and point
case checks. Stack ASan and LeakSanitizer remain disabled. Both revised scoped
constant headers pass strict C89 syntax checking. No new external SWF oracle
or foreign-OS runtime acceptance is claimed.


v0.379: indexed MovieClip width/height reads use parent-space vector bounds,
including descendant transforms and singular ancestors. Traversal snapshots and
native barriers protect reentrant removal; coercion GC, errors/OOM, aliasing and
interpreter/explicit/automatic JIT are covered. Writes and full resource/stroke/
named dimension semantics remain open. [EN](MOVIECLIP_INDEXED_SIZE.md) /
[RU](MOVIECLIP_INDEXED_SIZE.ru.md). Все стадии остаются открытыми.

Validation v0.379: all 785 AGG / 785 Cairo / 766 minimal Linux Release checks
have passing evidence after nine executable-permission repairs on eight ELF
files and targeted reruns (four repair operations AGG, three Cairo, two minimal).
Eight restricted heap/global ASan+UBSan checks pass: the four new dimension
checks plus standard MovieClip, AVM1/JIT display properties and geometry
allocation. Stack ASan and LeakSanitizer remain disabled. No new external SWF
oracle or foreign-OS runtime acceptance is claimed. All nine stages remain open.


v0.380: SWF4–6 registerClass linkage names use shared Flash unit folding for
registration, C lookup/removal and native attach/duplicate/fallback selection.
Case replacement publishes constructor and new spelling atomically; GC/transfer,
OOM/retry and interpreted/explicit/automatic CallMethod are covered. Separate
global environments and exact mixed-movie provenance remain open.
[EN](OBJECT_REGISTER_CLASS_CASE.md) / [RU](OBJECT_REGISTER_CLASS_CASE.ru.md).
Все стадии остаются открытыми.

Validation v0.380: all 789 AGG / 789 Cairo / 770 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new registry/scene/JIT cases plus existing
C89 registry, name/transfer, sprite resources and duplicate-version checks.
Stack ASan and LeakSanitizer remain disabled. No new original external SWF
oracle or foreign-OS run is claimed. All nine roadmap stages remain open.


v0.381: loaded InstanceOf replaces the 4096-node rejection/linear visited
scan with an identity hash set and shared native execution accounting. Cyclic
8192-node true/false graphs, nested/unlimited/saturated budgets, OOM/GC/retry
and large interpreter/explicit/automatic native membership are covered. Array
retains its accounting through the shared scope alias. Ordinary prototype bounds
and host policy remain. [EN](INSTANCE_OF_GRAPH_WORK.md) /
[RU](INSTANCE_OF_GRAPH_WORK.ru.md). Все стадии остаются открытыми.

Validation v0.381: all 792 AGG / 792 Cairo / 773 minimal Linux Release checks
have passing evidence; AGG/Cairo passed initially, minimal required one ELF
execution-permission repair and targeted rerun. Eight restricted heap/global
ASan+UBSan checks pass: three new graph/native checks, existing membership
callbacks/allocation/data and large-sort/negative-end Array regressions.
Stack ASan and LeakSanitizer remain disabled. The new native polling and revised
membership constants headers pass a strict C89 syntax probe. No new original
external SWF oracle or foreign-OS acceptance is claimed. All nine stages remain open.


v0.382: loaded String.split removes its 65536-field runtime rejection;
loaded split/indexOf/lastIndexOf share nested execution work for scan positions,
comparisons and split UTF-16 copies. Large results, failed long prefixes,
finite/unlimited/saturated/nested budgets, OOM/GC/retry and interpreter/native
calls are covered. Host bounds and existing SWF delimiter/limit rules remain.
[EN](STRING_LOADED_WORK.md) / [RU](STRING_LOADED_WORK.ru.md).
Все стадии остаются открытыми.

Validation v0.382: all 796 AGG / 796 Cairo / 777 minimal Linux Release checks
have passing evidence after three ELF execution-permission repairs and targeted
reruns (two AGG, one minimal; Cairo passed initially). Eight restricted heap/
global ASan+UBSan checks pass: four new work/allocation/callback/native cases
plus legacy split/search/receiver and character-allocation checks. Stack ASan
and LeakSanitizer remain disabled. The revised String constants header passes
a strict C89 syntax probe. No new original external SWF oracle or foreign-OS
run is claimed. All nine roadmap stages remain open.


v0.383: SWF7–8 interface snapshot capture/inspection shares native AVM1 work;
budget/OOM failure preserves edges and first-installation state. Validated
loaded zero-count/primitive/SWF4–6 noops avoid direct-service root allocation.
The full 65536-reference bound, GC, saturation, allocation sweeps and interpreted/
explicit/automatic native failure/retry are covered. Host/count policies remain.
[EN](IMPLEMENTS_WORK.md) / [RU](IMPLEMENTS_WORK.ru.md).
Все стадии остаются открытыми.

Validation v0.383: all 799 AGG / 799 Cairo / 780 minimal Linux Release checks
have passing evidence after five ELF execution-permission repairs and targeted
reruns (two AGG, three Cairo; minimal passed initially). Eight restricted heap/
global ASan+UBSan checks pass: three new declaration/work cases, existing
snapshot allocation/transport, membership data/large-graph and collecting
membership-callback regressions. Stack ASan and LeakSanitizer remain disabled.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.


v0.384: loaded Number.toString nondecimal values follow pinned Int32 truncation
and MIN-sentinel byte formatting without signed overflow. All 34 bases, fractions,
range/NaN/Infinity inputs, captured radix coercion, OOM/GC/transfer and native/
interpreted results are covered. Decimal and unloaded host formats remain.
Historical player-specific behavior is not certified.
[EN](NUMBER_LOADED_RADIX.md) / [RU](NUMBER_LOADED_RADIX.ru.md).
Все стадии остаются открытыми.

Validation v0.384: all 803 AGG / 803 Cairo / 784 minimal Linux Release checks
have passing evidence after nine ELF execution-permission repairs and targeted
reruns (four AGG, four Cairo, one minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new radix/callback/allocation/native cases plus
legacy radix/conversion, C89 standard Number and native numeric regressions.
Stack ASan and LeakSanitizer remain disabled. The new scoped radix constants
header passes a strict C89 syntax probe. No original external SWF oracle or
foreign-OS acceptance was added. All nine roadmap stages remain open.


v0.385: SetProperty width/height computes both intrinsic scales from
untransformed vector/descendant bounds using pinned rotation rules. Singular
ancestors, degenerate geometry, coercion GC/detachment, atomic OOM/errors and
interpreted/explicit/automatic native writes are covered. Named dimensions and
full geometry conformance remain open.
[EN](MOVIECLIP_DIMENSION_WRITES.md) / [RU](MOVIECLIP_DIMENSION_WRITES.ru.md).
Все стадии остаются открытыми.

Validation v0.385: all 807 AGG / 807 Cairo / 788 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo, three minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new dimension write/allocation/callback/native
cases plus previous dimension reads, AVM1 display properties, native indexed
reads and geometry allocation. Stack ASan and LeakSanitizer remain disabled.
The revised MovieClip constants header passes a strict C89 syntax probe.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.


v0.386: loaded String range copies charge each emitted UTF-16 unit with shared
nested work, saturation/error atomicity and missing-argument noops.
RU: учёт копирования slice/substring/substr; все девять стадий открыты.


v0.387: concat/fromCharCode charge argument visits and UTF-16 output, sharing
work with callbacks. RU: общий бюджет построения строк; все стадии открыты.


v0.388: public loaded String casing charges UCS2 mapping and UTF-16 output
with shared work; internal property folding keeps its caller policy.
RU: учёт публичного изменения регистра; все девять стадий открыты.


v0.389: loaded charAt/charCodeAt share receiver/index callback work and charge
one extracted UTF-16 unit. RU: общий бюджет извлечения символа; стадии открыты.


v0.390: independent traced legacy/exact registerClass maps survive reload, GC,
channel transport and replacement OOM. Старые плоские записи сохраняют прежнюю
политику; полное разделение globals/base-clip provenance открыто. Все стадии открыты.


v0.391: retained source-movie metadata selects registerClass environment from
the activation base clip, independently of SetTarget/ambient version.
RU: версия базового клипа для регистрации; старые графы используют fallback. Стадии открыты.


v0.392: attach/duplicate/authored placement choose registered constructors by
source symbol version, including transported fallback maps.
RU: версия символа при создании экземпляров; старые библиотеки используют fallback. Стадии открыты.


v0.393: counted TextField storage preserves embedded NUL through layout, UTF-16
range edits, GC and binary/queue graph transport. RU: хранение текста с NUL.
Strict scalar text and native size bounds are retained; full TextField compatibility remains open.


v0.394: TextFormat validates complete counted alignment/font/URL/target values.
RU: скрытый NUL не обходит ограничения форматирования; стадии остаются открытыми.


v0.395: TextField graph restore enforces native byte/scalar storage bounds
before native/font allocation. RU: проверка повреждённых графов и отката. Стадии открыты.


v0.396: identical counted loaded TextField text assignment preserves current
format/layout; changed text applies future format. RU: повторное присваивание
сохраняет формат; host-политика прежняя. Стадии остаются открытыми.


v0.397 — этапы 6/7/9: явно включаемая передача функций интерфейсами оригинала,
включая C callback без кодека; существующий порт и поток брокера, ID цели/источника/
запроса, результаты и исключения. Эксклюзивная передача бездействующего контекста
обязательна; постоянное соединение и реентерабельные вызовы остаются открытыми.
См. REMOTE_FUNCTIONS.md / .ru.md. Все стадии открыты.

v0.398 — этапы 6/7: строгая проверка тела ответа функции; циклические аргументы,
алиасы получателя и возврат интерфейса C-функции. Все стадии открыты.

v0.399 — этапы 6/7/8: выделение URI до публикации цели, отказы памяти без лишних
корней и GC внутри оригинального prepared body с самоснятием привязки.
Откат экспортов всего графа и все стадии остаются открытыми.

v0.400 — этапы 6/7: вложенные интерфейсы функций одного брокера исполняются
на его потоке с графами и защитой оригинальных куч; предел 32 активных вызова.
A→B→A и возврат ошибок проверяются отдельно; циклы разных брокеров и все стадии открыты.

v0.401 — этапы 6/7/9: учебный пример C89 и запуск C callback без кодека между
двумя независимыми процессами; URI, ответ, NUL-исключение, статусы и исходная
userdata. Межплатформенная приёмка и все стадии остаются открытыми.

v0.402 — этапы 6/7/9: примеры между процессами, независимыми VM в разных потоках
и компьютерами LAN; настраиваемые bind/advertised host/port. Локальные проверки
не заменяют физические LAN/иные ОС; все стадии остаются открытыми.

v0.403 — этапы 6/7: отзыв одной исходной функции, новый URI при повторном
экспорте и фиксированные токены для удаления записей брокера без выделений.
Чужие цели/корни сохраняются; полный откат графа и все стадии остаются открытыми.

v0.404 — этап 9, документация: самостоятельные README EN/RU внутри примера
function_rpc: назначение, требования, сборка и три схемы запуска, CTest.
Исполнение и статусы стадий не изменены.

v0.405 — этапы 6/7: остановка с постоянными клиентскими контекстами мутексов
без выделений ключей; удерживаемые блокировки освобождаются, корни функций
сохраняются до отключения источника. Все стадии остаются открытыми.

v0.406 — этапы 6/7/9: граница C API при создании брокера удерживает исключения
конструкторов; OOM, занятый порт и повторный запуск проверяются отдельно.
Межплатформенная приёмка и все стадии остаются открытыми.

v0.407 — этапы 6/7/9: полная побайтовая проверка входящих ID и контекста
источника до создания peer/вызова функции; сырые TCP-пакеты и восстановление
проверяются отдельно. Все стадии остаются открытыми.

v0.408 — этап 9: VERSION.txt становится входом конфигурации CMake; обычная
сборка после изменения только версии обновляет метаданные. Проверка использует
рабочий модуль и временный проект, не изменяя репозиторий. Все стадии открыты.

### v0.409 — UTF-16 count memory / Память подсчёта UTF-16

Implemented allocation-free count queries and boxed String length validation;
see [UTF16_STRINGS.md](UTF16_STRINGS.md). Проверка без выделений добавлена;
этапы 1–9 остаются открытыми. Linux minimal Release: 822/822 CTests passed. Generated native-test launchers
restore executable mode before exec because the workspace resets file modes;
test bodies and assertions are unchanged. Other-platform acceptance remains open.

### v0.410 — UTF-16 buffer preflight / Проверка ёмкости UTF-16

Valid short buffers now publish required length without allocation or writes;
malformed input and overlapping copy behavior are preserved.
[Details / Подробности](UTF16_STRINGS.md). All nine stages remain open.

Validation v0.410 / Проверка: Linux minimal Release 822/822; AGG Release
4/4 focused string tests; heap/global ASan+UBSan 4/4. Generated native launchers
restore workspace executable permissions; stack ASan and LSan remain disabled.
No other-platform runtime or complete stage acceptance is claimed.

### v0.411 — one UTF-16 snapshot allocation / Одно выделение снимка

The sufficient-capacity C API copy reserves the measured UTF-16 size exactly,
removing geometric reallocation and transient old/new decoded arrays. Snapshot
semantics for overlapping input/output remain. A 257-unit regression allowing
exactly one allocation fails on v0.410 and passes after this change.
Копирование резервирует точный размер снимка одним выделением памяти; перекрытие
буферов поддерживается. Все девять стадий остаются открытыми.

Validation v0.411 / Проверка: minimal Release 822/822; AGG Release 4/4;
heap/global ASan+UBSan 4/4 string tests. Strict C89 public-header syntax passed.
Native test launchers restore executable modes; stack ASan/LSan are disabled.
Other platforms and complete Flash compatibility remain unverified.

### v0.412 — loaded parseInt / Загруженный parseInt

Separate SWF4–8 parsing now uses radix-before-text coercion, explicit-base
validation, raw hex/octal and counted-prefix rules; unloaded host behavior stays.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.
User reconfirmed SWF ≤8 only on 16 October; AVM2/AS3/SWF9+ are outside this scope.

Validation v0.412 / Проверка: Linux minimal Release 823/823 CTests; AGG Release
and heap/global ASan+UBSan each passed all three global-parsing executables.
Native launchers restore workspace executable modes; stack ASan/LSan disabled.
Other-platform runtime and all nine stage acceptance remain open.

### v0.413 — loaded parseFloat / Загруженный parseFloat

Separate SWF4–8 decimal accumulation now preserves pinned decimal rounding,
multiple-dot, missing-argument and wrapped-exponent behavior with counted scans.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All stages remain open.

Validation v0.413 / Проверка: minimal Release 824/824; AGG Release and
heap/global ASan+UBSan each passed all four global-parsing executables.
Native mode-restoring launchers used; stack ASan/LSan disabled.
Other-platform runtime acceptance and all nine stages remain open.

### v0.414 — parsing work/GC/JIT / Бюджет, GC и JIT разбора

Loaded numeric scans share nested AVM1 work and saturated counters. Exact quota,
allocation-failure cleanup and real interpreted/attached/automatic native call
regressions added. [EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md).
All nine stages remain open; no complete decoder/coercion budget is claimed.

Validation v0.414 / Проверка: minimal Release 827/827; AGG Release 7/7 and
heap/global ASan+UBSan 7/7 parsing/JIT tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. All nine stage acceptances remain open.

### v0.415 — numeric leading whitespace / Ведущие пробелы чисел

Loaded Number/ToNumber and parseFloat now share Flash's four-unit leading-space
policy; host rules stay. SWF4 invalid-zero vs SWF5–8 NaN, counted surrogate/NUL,
traced GC and actual native entry regressions added.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.415 / Проверка: minimal Release 829/829; AGG Release 9/9 and
heap/global ASan+UBSan 9/9 parsing/whitespace/JIT tests. Native mode-restoring
launchers used; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.416 — shared decimal coercion / Общее десятичное преобразование

Loaded Number/ToNumber/numeric operands share reference-ordered decimal
accumulation with parseFloat; strict SWF5–8, relaxed SWF4 and raw integer radix
policies retained. Primitive decimal conversion removes temporary allocations.
[EN](GLOBAL_PARSING.md) / [RU](GLOBAL_PARSING.ru.md). All nine stages remain open.

Validation v0.416 / Проверка: minimal Release 832/832; AGG Release 12/12 and
heap/global ASan+UBSan 12/12 parsing/decimal/whitespace/JIT tests. Native launchers
restore workspace executable modes; stack ASan/LSan disabled. No other-platform
runtime acceptance or complete stage closure is claimed.

### v0.417 — apply preparation roots / Корни подготовки apply

Boxed receiver and partially captured arguments survive real script-getter GC.
Counted strings, cycles, errors/exceptions and retry covered in host/SWF4–8.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.417 / Проверка: minimal Release 833/833; AGG Release 2/2 and
heap/global ASan+UBSan 2/2 apply/function tests. Generated native launchers restore
workspace executable modes; stack ASan/LSan disabled. Foreign runtime acceptance
and all nine stages remain open.

### v0.418 — loaded apply length / Длина загруженного apply

Primitive lists supply zero arguments; own raw length uses version casing and
AVM1 Int32 conversion, with rooted collecting callbacks. Loaded negative/large
lengths remain explicitly bounded. [EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md).
All nine stages remain open.

Validation v0.418 / Проверка: minimal Release 834/834; AGG Release 3/3 and
heap/global ASan+UBSan 3/3 apply/function tests. Native launchers restore workspace
executable modes; stack ASan/LSan disabled. Other-platform acceptance remains open.

### v0.419 — apply property capture / Чтение свойств apply

Loaded native Number/Boolean/String/Date lists use ordinary named properties,
original getter receiver and retained snapshots across mutation/GC/failure.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.419 / Проверка: minimal Release 835/835; AGG Release 4/4 and
heap/global ASan+UBSan 4/4 apply/function tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. Other-platform runtime acceptance remains open.

### v0.420 — incremental apply work / Постепенный apply с бюджетом

Loaded positive Int32 lists grow rooted capture under shared work; the former
65536 cap is removed. C/script 65537 slots, huge bounded allocation, OOM/unwind
and real native CallMethod checks added. Host/negative-length policy retained.
[EN](FUNCTION_APPLY.md) / [RU](FUNCTION_APPLY.ru.md). All stages remain open.

Validation v0.420: Linux minimal 838/838; AGG 7/7; heap/global ASan+UBSan 7/7.
Generated native-mode launchers compensate for workspace executable-mode resets.
Stack ASan and LeakSanitizer are disabled in this environment. Other platforms
and completion of all nine stages remain unverified/open.

## v0.421 — заимствованные адаптеры и невызываемые цели

В загруженных SWF4–8 заимствованные Function.call/apply возвращают undefined,
если у цели нет callback вызова. Упаковка this и подготовка длины/индексов apply
выполняются раньше: ошибки getter, исключения со встроенным NUL и исчерпание
квоты продолжают передаваться вызывающему. Host API сохраняет UNSUPPORTED;
прямой публичный вызов и C/script/удалённые вызываемые интерфейсы не меняются.
Основание — закреплённая версия Ruffle function.rs по ссылке выше.

function_borrowed_adapters воспроизводит прежний сбой и проверяет версии SWF4–8,
host, настоящую сборку GC внутри script getter, исключения, повторный вызов,
вызываемую цель и порядок проверки бюджета. jit_function_apply дополнен обоими
адаптерами через интерпретатор и два режима JIT с входом в машинный код.
Полная семантика super и Function, как и все девять стадий, остаётся открытой.

Проверено v0.421: Linux minimal 839/839; AGG 9/9; heap/global ASan+UBSan
9/9, включая регрессию удалённых функций. Использованы генерируемые средства
восстановления права запуска; stack ASan/LeakSanitizer отключены. Другие ОС
не проверены; все стадии остаются открытыми.

## v0.422 — super требует прямой диспетчеризации

Загруженные Function.call/apply считают сохранённый объект super не функцией,
несмотря на внутренний callback для прямого вызова родителя. Подготовка аргументов
выполняется до undefined: script getter индекса запускается, может собирать GC,
передаёт ошибки и исключения со встроенным NUL. Заимствованный host-вызов и прямой
публичный вызов объекта super сохраняют прежнюю диспетчеризацию родителя.
Основание — явное разделение as_function и прямого super в закреплённом
function.rs Ruffle. Полная семантика super/Function не заявляется.

function_super_adapters воспроизвёл сбой на v0.421; проверены SWF5–8 и host,
оба адаптера, побочные эффекты родителя, GC в getter, ошибки/исключения, повторный
и прямой вызовы, восстановление корней и бюджета. Отрицательная signed длина
apply остаётся UNSUPPORTED: эталон переводит её в размер выделения, поэтому
считать её нулём по этому источнику нельзя. Все стадии остаются открытыми.

Проверено v0.422: Linux minimal 840/840; AGG 13/13; heap/global ASan+UBSan
13/13, включая прямой super, владельца/прототип и удалённые функции. Применены
генерируемые средства восстановления права запуска. Stack ASan/LeakSanitizer
отключены; дополнительные ОС и завершение стадий не подтверждены.

## v0.423

В v0.423 общий поиск сохранённого имени использует существующее сравнение UTF-16
по закреплённой таблице нижнего регистра Flash без дополнительных выделений.
В SWF4–6 hasOwnProperty, чтение флагов и обычное чтение по цепочке прототипов
принимают эквивалентный регистр. Host и SWF7–8 сохраняют точное сравнение.
Проверены таблицы данных, аксессоров и нативных методов; запрос наличия не
запускает getter. Обёртки методов используют сохранённое имя и сохраняют
идентичность Function. Запись существующего собственного свойства выбирает его
сохранённое имя; вариант стандартного __proto__ — каноническое имя, без дубликата.

object_property_case_queries воспроизвёл ошибку v0.422. Проверены SWF4–8/host,
ASCII, U+00C4/U+00E4, NUL и одиночный суррогат в имени, наследуемое чтение,
собственное наличие, скрытые флаги, отсутствие вызова getter/метода, идентичность
Function, изменение значения и GC. Основание — script_object.rs Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd; ссылка в PROPERTY_CASE.md.

Полная совместимость не заявляется. Для дубликатов, созданных raw/internal API,
приоритет точного имени и порядка map ещё не сверен с Flash. Изменение флагов,
удаление, наследуемые setter, watcher, API определения, метатаблицы расширений и
наличие свойств super требуют согласованных правок. Линейный поиск ещё не полностью
учитывается бюджетом. Исходный бинарный SWF не удалось восстановить через reader
для текста, поэтому число прошедших исходных SWF не повышается. Все стадии открыты.

В SWF5+ убрана искусственная обычная запись this: поиск через ячейку активации
сохраняет особое правило локального this в SWF5 (function.rs load_this задаёт
только регистры). Прежние this/interpreter/JIT тесты не изменены. Тест координат
теперь ожидает регистронезависимое публичное чтение SWF4–6, сохраняя проверки
преобразования. GC-тест классов использует ParentClass вместо Super: последнее
имя теперь корректно совпадает с локальным неявным super.

Проверено v0.423: Linux minimal 841/841; AGG 8/8; heap/global ASan+UBSan
8/8. Применены генерируемые средства восстановления права запуска; stack ASan
и LeakSanitizer отключены. Другие платформы и все стадии остаются открытыми.

## v0.424 — изменение выбранного собственного имени

Изменение флагов и удаление выбирают сохранённое собственное имя до изменения
таблиц. В SWF4–6 вариант регистра теперь изменяет/удаляет то же данное, аксессор
или метод, которое находят чтение и hasOwnProperty. DONT_DELETE проверяется до
удаления; успешное удаление очищает соответствующие флаги. Наследуемые свойства
не удаляются. Имя копируется до очистки таблиц внутри границы обработки отказов
памяти C API; запись использует тот же общий selector. Host/SWF7–8 сохраняют
точный регистр и прежнюю политику API флагов без соответствующего свойства.

object_property_case_mutation воспроизвёл ошибку v0.423. Проверены SWF4–8/host,
данные/аксессоры/методы, NUL/Unicode, DONT_DELETE, снятие флага/повторное удаление,
оставшиеся флаги, наследуемое владение, отсутствие callback и GC. Setter/watch,
определение имён, коллизии, бюджет и полный super ещё открыты; стадия не закрыта.

Проверено v0.424: все 842 случая Linux minimal прошли: сначала 840; два
процессных теста получили PermissionError запуска дочернего файла, после
восстановления права запуска повтор прошёл 2/2. AGG 6/6; heap/global ASan+UBSan
6/6. Stack ASan/LeakSanitizer отключены. Другие ОС и все стадии открыты.

## v0.425 — наследуемый setter и нативное исполнение

Поиск setter теперь выбирает данные и аксессоры с теми же правилами регистра,
что чтение. Собственные данные сохраняют приоритет над наследуемым setter;
host/SWF7–8 остаются точными. Активация setter сохраняет имя фактического аксессора,
а не вариант запроса: повторный поиск script-дескриптора и корни относятся к
выбранному владельцу даже при удалении аксессора и отсоединении прототипа.

object_property_case_setter воспроизвёл ошибку v0.424. Проверены SWF4–8/host,
имена и значения с NUL, this/владелец/имя, реальный GC, отсоединение владельца и
удаление дескриптора, ошибки, исключения и очистка состояния. jit_property_case_setter
проверяет те же случаи через SetMember в интерпретаторе и двух режимах JIT с
фактическими входами в машинный код. object_property_case_allocation перебирает
отказы выделения при флагах/удалении/записи до успешного выполнения и проверяет
неизменность данных/флагов при OOM, восстановление корней и metadata setter.
Watcher, определение свойств, порядок дубликатов, полная семантика областей/имён
и учёт поиска бюджетом ещё требуют работы. Все девять стадий остаются открытыми.

Проверено v0.425: Linux minimal 845/845; AGG, NO_OS_PORTS и heap/global
ASan+UBSan — по 7/7. Входы JIT проверены на Linux x64; NO_OS_PORTS собирался
и исполнялся на Linux, это не запуск другой ОС. Использованы средства
восстановления права запуска; stack ASan/LeakSanitizer отключены. Стадии открыты.

## v0.426 — собственные свойства через super

В загруженном режиме запрос собственных свойств super обращается к сохранённому
исходному объекту, как has_own_property в закреплённом script_object.rs. Запрос
не читает приватные данные фасада и свойства родителя, сохраняет правила
регистра версии SWF. Host-поведение прежнее. Цепочки вложенных/повреждённых
фасадов обходятся итеративно с пределом; цикл возвращает [FVM_ERROR](../include/flashvm/flashvm.h#L43).

object_super_membership воспроизвёл сбой v0.425. Проверены SWF5–8 и host, C API и
заимствованный Object.hasOwnProperty.call, NUL в имени, регистр, наследуемые и
отсутствующие свойства. Getter AVM1 действительно бросает исключение при чтении,
но запрос наличия его не запускает. Проверены GC, отказ на повреждённом цикле,
восстановление и перенос через очередь в другой контекст с повторным GC/запросом.
Полные перечисление/флаги/удаление/определение super, независимая сверка Flash,
другие транспорты/платформы и все стадии остаются открытыми. Изменён только запрос.

Проверено v0.426: Linux minimal 846/846; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Независимый Flash и другие ОС не проверены.

## v0.427 — выбор имени watcher

В SWF4–6 замена watch, вызов, получение старого значения без getter и unwatch
используют эквивалентность имён Flash. Callback получает запрошенное написание
полного имени, а не сохранённый регистр свойства. Собственное имя для записи
выбирается после callback по текущей таблице. Host/SWF7–8 сохраняют точный регистр.
Старые virtual-данные, числовые слоты и ленивые методы читаются без getter.

object_property_case_watch воспроизвёл ошибку v0.426. Проверены SWF4–8/host,
NUL, аргументы old/new/userData, замена без второго watcher, unwatch с другим
регистром, самоудаление перед настоящим GC, ошибки/исключения и очистка состояния.
Основание — закреплённые script_object.rs watch/unwatch/call_watcher, передающие
имя запроса в Watcher.call. Запись при исключении, идентичность при рекурсии,
коллизии, полный MovieClip watch и бюджет поиска остаются открытыми. Прежняя
политика записи при исключении здесь сохранена; все стадии остаются открытыми.

Проверено v0.427: Linux minimal 847/847; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка стадий открыты.

## v0.428 — замена virtual-свойства с учётом регистра

В SWF4–6 addProperty выбирает сохранённое собственное имя до проверки флагов и
замены дескриптора. Данные и флаги сохраняются; вариант имени не создаёт второе
virtual-свойство рядом с прежним данным. Host/SWF7–8 сохраняют разные имена.
Прежние правила null-setter и отката сохранены. Основание — закреплённый
script_object.rs add_property_with_case.

object_property_case_definition воспроизвёл ошибку v0.427. Проверены SWF4–8/host,
NUL, флаги/данные, GC в getter, отсутствие преждевременного getter и запись данных
с null-setter. Перебор отказов памяти проверяет откат и последующее успешное
определение. jit_property_case_watch проверяет имена watcher, самоудаление/GC и
ошибки через интерпретатор и два режима JIT с реальными нативными входами.
Нативные C-определения, коллизии, запись при исключении watcher, полные свойства
и все стадии ещё открыты.

Проверено v0.428: Linux minimal 850/850; AGG, NO_OS_PORTS на Linux и heap/global
ASan+UBSan — по 8/8. Использованы средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Другие ОС и завершение стадий не подтверждены.

## v0.429 — C-определения с корнями и сохранённые имена

Определение нативного метода/аксессора с корнем выбирает существующее собственное
имя SWF4–6 перед прежней проверкой конфликтов. Замена больше не создаёт скрытый
дубликат регистра; конфликт с данными/аксессором отклоняется так же, как при
точном имени. Host/SWF7–8 сохраняют разные имена. C89-сигнатуры и ABI прежние.

object_property_case_native воспроизвёл дубликат v0.428. Проверены SWF4–8/host,
NUL, замена callback/корней, идентичность записей метода/аксессора, отказы при
конфликте с данными, настоящий GC и вызов прежней закреплённой Function с её
старым callback/корнем. Это контракт C-расширений FlashVM, согласованный с
загруженным поиском, а не заявление о C API Adobe. Устаревший внутренний кеш
методов, полное время жизни callbacks/коллизии, исключения watcher и стадии открыты.

Проверено v0.429: Linux minimal 851/851; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.430 — корни кеша методов

В v0.430 устаревшая ленивая обёртка Function удаляется из кеша после успешной
замены нативного метода, удаления, перезаписи обычным значением или замены нативным
аксессором. Затенённая данными/аксессором запись метода тоже удаляется. Очистка
идёт после выделений и публикации: OOM сохраняет прежние запись и обёртку.
Одинаковые callback/userdata/root сохраняют идентичность Function.

[GC:] Сохранённая или закреплённая Function владеет собственным корнем callback.
Очистка кеша владельца не меняет её поведение. Невостребованная обёртка больше
не удерживает старый корень только через закреплённый объект-владелец. Правило
действует в host и загруженных контекстах после прежних проверок флагов/удаления.
Публичный ABI не меняется.

gc_method_cache воспроизвёл сбой v0.429. Проверены host/SWF4–8, четыре операции,
старая Function с pin и без pin, одинаковое повторное определение, настоящий GC,
проверка реестра живых объектов до разыменования, вызов старого callback и
освобождение после unpin. gc_method_cache_allocation перебирает отказы до успеха
в host/SWF6/SWF8, проверяет прежние данные/Function/корень при OOM и освобождение
после успеха.

Полные GC/callback не заявляются: кеши глобальной регистрации, остальные ресурсы,
воскрешение finalizer и конкурентные RPC требуют своей приёмки. Все стадии открыты.

Проверено v0.430: Linux minimal 853/853; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Использованы средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.431 — длина Array в watcher с другим регистром

Чтение старого значения без getter распознаёт ресурсную длину Array по правилам
полного имени Flash. В SWF4–6 запись LENGTH при watch length получает старую длину
3, имя LENGTH и новое значение 5; возврат 4 меняет длину массива на 4. SWF7–8
сохраняют разные имена; host-политика watch нативной длины прежняя. Суффикс с NUL
не становится length и не запускает watcher.

array_case_watch_length воспроизвёл ошибку v0.430. Проверены host/SWF4–8,
аргументы, ресурсная длина без getter, настоящий GC, изменение длины и NUL.
jit_array_case_watch_length повторяет SetMember в интерпретаторе и двух режимах
JIT с проверкой настоящих нативных входов. Полные переопределения length,
запись при исключениях, бюджет и все стадии остаются открытыми.

Проверено v0.431: Linux minimal 855/855; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Использованы средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.432 — исключения watcher

В v0.432 присваивание загруженных Object/Array продолжается после исключения
script-watcher со значением undefined. Собственная запись возвращает исходное
исключение; readonly-данные не меняются, но исключение передаётся. Собственный
virtual-setter получает undefined, и исходное исключение watcher имеет приоритет
над последующим исключением setter. Наследуемый virtual-setter возвращает свой
результат: успех поглощает исключение watcher, собственное исключение передаётся.
Host-политика ошибок watcher прежняя.

Основание — закреплённые object.rs set_internal и script_object.rs call_watcher
Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd; ссылки в WATCH_THROW.md.
Первый продолжает присваивание и отдельно обрабатывает наследуемый setter;
второй заменяет присваиваемое значение на undefined при script-исключении.

[GC:] [FVM_WatchWriteScope](../src/core/watch_write_scope.hpp#L8) сохраняет граф исключения до очистки pending-состояния.
Явный корень защищает строки, объекты и циклы во время setter/GC. Деструктор
восстанавливает владение pending-значением до снятия корня и выполняется раньше
окружающего корня владельца поиска. Отказ выделения до приостановки сохраняет
исходное pending-исключение. При обычной записи вектор сохранения не выделяется.

object_watch_throw воспроизвёл ошибку v0.431. Проверены host/SWF4–8, именованные
данные, числовые слоты Object/Array, readonly, собственный/наследуемый setter,
успех/исключение setter и приоритет ошибок. Самоцикличный объект исключения
переживает настоящий GC внутри setter, где pending-состояние приостановлено.
Прежние тесты строк с NUL и JIT теперь ожидают запись undefined при loaded throw.
[FVM_ERROR](../include/flashvm/flashvm.h#L43) и OOM по-прежнему останавливают операцию; их эквивалентность ошибкам
эталона не заявляется. Нативная длина/ресурсы, метатаблицы, пределы рекурсии и
полная совместимость требуют проверок. Все девять стадий остаются открытыми.

Проверено v0.432: Linux minimal 856/856; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Использованы средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и все стадии открыты.

## v0.433 — рост Array не выполняется для наследуемого virtual-setter

Перед увеличением длины индексная запись загруженного Array теперь определяет,
принадлежит ли virtual-свойство прототипу. Наследуемые setter, включая virtual
только с getter, не переходят в Array.set_local и не увеличивают массив.
Собственные записи сохраняют рост до readonly-проверки и собственного setter.
Это уточняет прежнее общее правило роста до setter. В закреплённом object.rs
наследуемый setter вызывается раньше native-роста script_object.rs set_local.

array_inherited_setter_length воспроизвёл сбой v0.432: наследуемый setter индекса
5 видел длину 6 вместо 0. Проверены host/SWF4–8, собственный/наследуемый setter,
настоящий GC, исключения setter/watcher и их приоритет; собственный слот при
наследуемом вызове не создаётся. Нативный тест повторяет loaded-случаи в
интерпретаторе и двух режимах JIT, проверяя реальные входы и длину внутри setter.
Полные эффекты Array, ресурсные ошибки, повреждённые графы и бюджет ещё открыты.
Завершение стадии и другие платформы не заявляются.

Проверено v0.433: Linux minimal 858/858; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Использованы средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и все стадии открыты.

## v0.434 — virtual только с getter и проверка отложенной ошибки

Именованная запись через наследуемое нативное virtual-свойство только с getter
теперь поглощает исключение watcher, как наследуемый call_setter без setter.
Собственное данное не создаётся, getter не выполняется. Собственное getter-only
свойство возвращает исключение watcher. Числовой путь уже работал по этому правилу.
object_watch_getter_only воспроизвёл расхождение v0.433: проверены SWF4–8,
собственные/наследуемые, числовые/именованные случаи, NUL-исключение и GC watcher.

jit_object_watch_throw исполняет матрицу данных/readonly/собственного/наследуемого
setter с цикличными объектами исключений и GC в интерпретаторе и двух режимах JIT,
проверяя реальные нативные входы. object_watch_throw_allocation перебирает отказы
до первого завершённого присваивания с исключением: host/SWF4–8 сохраняют старые
данные при OOM, уже pending-исключение при отказе сохранения или подготовки,
очищают состояние и повторяют вызов. Проверяются payload и корни/watch/setter
после каждой границы. Это проверки собственных raw-записей, не всех отказов
памяти внутри setter. Остальные ошибки, ресурсы, рекурсия/коллизии и все стадии
открыты; правила нативного аксессора относятся к контракту C-расширений.

Проверено v0.434: Linux minimal 861/861; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.435 — заимствованное определение свойств super

В загруженном режиме Object.addProperty и Object.watch с явно переданным this =
super принимают корректные аргументы, но не сохраняют дескриптор/watcher на фасаде.
Object.unwatch возвращает false. Преобразование имени и проверки выполняются,
включая ошибки callback преобразования. Watch не принимает super как callback:
внутренний прямой вызов родителя не делает его обычной Function. Host-поведение
сохранено. Прямой super.method по-прежнему привязывает исходный объект; здесь
исправлены явно заимствованные методы с this = super.

object_super_definitions воспроизвёл запись аксессора v0.434. Проверены SWF5–8,
host, NUL, допустимые/ошибочные аргументы, число преобразований, исключения
преобразования, отсутствие дескрипторов/watcher, отказ callback и GC. Основание —
закреплённые globals/object.rs add_property/watch/unwatch и script_object.rs
add_property_with_case/watch/unwatch, отказывающиеся менять NativeObject::Super.
Полная рефлексия фасада, native API, callback аксессора и стадии остаются открытыми.

Проверено v0.435: Linux minimal 862/862; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Использованы средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.436 — super не является Function callback virtual-свойства

В загруженных script-свойствах getter/setter больше не вызывают фасад super как
обычную Function. Нефункциональный getter возвращает сохранённые данные;
нефункциональный setter не имеет исполняемого эффекта, но writable-данные
обновляются. Super остаётся допустимым Object-аргументом addProperty. Host и
явный прямой вызов родителя прежние. Закреплённые script_object.rs
call_getter/call_setter требуют as_function; Property хранит данные отдельно.

object_super_accessors воспроизвёл сбой v0.435. В host/SWF5–8 побочный эффект
конструктора обнаруживает ошибочный вызов родителя при get/set. Проверены GC
дескриптора с фасадом, сохранение прямого super, перенос через очередь в другой
контекст с повторным GC и проверкой данных/побочных эффектов. jit_object_super_accessors
повторяет GetMember/SetMember в интерпретаторе и двух режимах JIT с настоящими
нативными входами. Полные super/рефлексия, RPC, эквивалентность всех ресурсов и
все девять стадий остаются открытыми.

Проверено v0.436: Linux minimal 864/864; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Другие ОС и общая приёмка открыты.

## v0.437 — написание свойства после замены setter

После собственного script-setter загруженные SWF4–6 повторно выбирают текущее
написание свойства перед проверкой readonly и сохранением данных. Callback
может удалить `Slot` и создать `sLOT` как обычные данные или virtual-свойство:
исходное значение сохраняется в замену без повторного вызова setter. Readonly
сохраняет прежние данные. Host и SWF7–8 используют точное написание. Основание —
закреплённый script_object.rs set_local: поиск property entry после setter
учитывает регистр по версии SWF.

object_setter_case_replacement воспроизвёл прежнюю запись дублирующего слота,
затем проверяет три замены, host/SWF4–8, строку с NUL, GC внутри setter и
последующий GC traced-строк. jit_object_setter_case_replacement повторяет
загруженные варианты в интерпретаторе и двух режимах JIT с настоящими нативными
входами SetMember. Замена нативным аксессором, общие коллизии и все девять стадий
остаются открытыми.

Validation v0.437: Linux minimal 866/866; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. All nine stages and foreign OS execution remain open.

## v0.438 — регистр raw length заимствованных Array-методов

В загруженных SWF4–6 заимствованные Array-методы находят собственные raw-данные
`LENGTH` как `length`, включая сохранённые данные virtual-свойства. Raw-запись
выбирает существующее написание перед проверкой readonly и обновлением данных
дескриптора. Getter/setter не вызываются. SWF7–8 различают регистр. Основание —
закреплённые script_object.rs length/get_data/set_length/set_data и
globals/array.rs push/splice.

array_borrowed_length_case воспроизвёл пустой slice при `LENGTH=2`, затем
проверяет slice/push/splice, обычные/virtual/readonly данные, SWF4–8, отсутствие
дублирующего имени, отдельное имя с NUL и счётчики callback.
jit_array_borrowed_length_case повторяет матрицу с настоящими нативными входами
CallMethod. Общая совместимость Array, остальных ресурсов и все стадии открыты.

Проверено v0.438: Linux minimal 868/868; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.439 — заимствованные Array-методы на super

Загруженный мост raw-записи собственных данных не меняет фасад super.
Array.push/unshift/splice, заимствованные через Function.call, больше не
создают на escaped super индексы или length. Возвращаемые значения и прямые
вызовы родителя сохраняют контракт; host-запись прежняя. В закреплённом
script_object.rs set_element/set_length явно ничего не сохраняют на
NativeObject::Super. Исправление относится к raw-записи Array, а не ко всем
обычным присваиваниям super и операциям C-расширений.

array_borrowed_super воспроизвёл изменение фасада, затем выполняет подряд
push/unshift/splice/slice/pop/shift/reverse в SWF5–8. Проверены результаты,
неизменность raw-хранилища и исходного экземпляра, настоящий GC после каждого
метода и очистка состояния. jit_array_borrowed_super повторяет все вызовы в
интерпретаторе и двух нативных режимах с проверкой настоящих входов. Полные
super/Array, транспорт и все девять стадий остаются открытыми.

Проверено v0.439: Linux minimal 870/870; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.440 — registerClass не принимает super-конструктор

Загруженный Object.registerClass отказывает фасаду super как конструктору до
преобразования linkage-имени. Вызываемый интерфейс родителя не является Function
конструктором. Некорректная регистрация сохраняет прежний конструктор. Host и
снятие регистрации через null/undefined прежние. Основание — закреплённый
globals/object.rs register_class, проверяющий as_function до преобразования.

object_register_class_super воспроизвёл прежнее принятие/вызов преобразования,
затем проверяет SWF5–8 и host, NUL в имени, число callback, подавление ошибки
преобразования при неверном конструкторе и её передачу при верном, сохранение
регистрации, успешный повтор, снятие через null и GC.
jit_object_register_class_super повторяет регистрацию через CallMethod в
интерпретаторе и двух режимах JIT с настоящими нативными входами. Общая семантика
конструкторов и все стадии остаются открытыми.

Проверено v0.440: Linux minimal 872/872; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.441 — TypeOf загруженного super

Общий TypeOf возвращает `object` для загруженного фасада super, сохраняя прямой
вызов родителя. Настоящие Function по-прежнему дают `function`, обычные экземпляры
— `object`. Host-поведение сохранено. Классификация не выделяет память;
закреплённый avm1/value.rs type_of проверяет as_function, а не общую вызываемость.

object_super_typeof воспроизвёл прежний результат `function`, затем проверяет
host/SWF5–8, запись C-helper в исходный [FVM_Value](../include/flashvm/value.h#L24), Function, экземпляры, GC и
повтор, отсутствие вызова конструктора и сохранение прямого super.
jit_object_super_typeof повторяет TypeOf в интерпретаторе и двух режимах JIT с
настоящими нативными входами. Остальные рефлексия/преобразования super и все
девять стадий остаются открытыми.

Проверено v0.441: Linux minimal 874/874; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.442 — строковый fallback super

Загруженный ToString возвращает `[type Object]` для super, если доступный через
родителя toString вернул не строку. Общая вызываемость не делает фасад Function;
настоящие Function сохраняют `[type Function]`. Строки с NUL и исключения
передаются без изменения. Host прежний. Закреплённый avm1/value.rs
coerce_to_string проверяет as_function после специального вызова toString.
Привязка receiver/owner остаётся отдельным вопросом; здесь исправлена только
классификация fallback.

object_super_string воспроизвёл прежний Function fallback. Проверены SWF5–8,
number/undefined/null/string/exception, число callback, C-helper с совпадением
входа/выхода, Function/экземпляр, GC и прямой вызов родителя.
jit_object_super_string повторяет ToString в трёх режимах с настоящими
нативными входами. Полные преобразования и все стадии остаются открытыми.

Проверено v0.442: Linux minimal 876/876; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.443 — receiver и owner специальных преобразований super

Загруженные числовое/default/строковое преобразования super используют вызов
родительского метода с исходным экземпляром и выбранным владельцем. Отсутствие
или невызываемое значение дают undefined; ошибки вызываемого метода передаются.
Поиск владельца учитывает регистр по версии SWF, а корень удерживает его до
getter, который может отсоединить предка. Ошибки явных вызовов и host-преобразования
прежние. Основание — закреплённые object.rs/super_object.rs call_method: Special
вызов использует super.this() и глубину родительского поиска.

object_super_coercion_receiver воспроизвёл передачу фасада вместо экземпляра.
Проверены ToNumber/ToString/default primitive в SWF5–8, регистр SWF5–6,
примитивные и невызываемые Object-методы, числа/строки с NUL, исключения/обычные
ошибки, GC в callback и удаление метода с отсоединением прототипа. Проверяются
receiver/owner и очистка состояния. jit_object_super_coercion_receiver повторяет
ToNumber/ToString/Add2 в трёх режимах с настоящими нативными входами. Общие
разрешение/мутации владельцев, отказы памяти и все стадии остаются открытыми.

Проверено v0.443: Linux minimal 878/878; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.444 — корни идентичности вызова без выделения памяти

[FVM_CallScope](../src/core/call_scope.hpp#L8) удерживает сохранённые/активные constructor, function и owner
через шесть встроенных root-frame. Публикация полной цепочки и восстановление
прежней идентичности больше не выделяют временный вектор. Вложенные вызовы и
раскрутка исключения сохраняют начало корней вызывающей стороны. Это изменение
GC/реализации, а не расширение заявленной совместимости SWF.

super_conversion_allocation воспроизвёл невозможность войти в call-scope при
полном запрете new/new[]. Теперь вход/выход и вложенная раскрутка работают при
этом запрете; GC удерживает шесть объектов и освобождает после отсоединения.
Также перебираются границы new/new[] до первого завершённого ToNumber/ToString/
Add2 или исключения в SWF5–8, с новым контекстом на каждой границе. Отказы
сохраняют совмещённый выход, очищают рекурсию/корни, удерживают имеющийся payload
исключения и позволяют GC/повтор. Это границы C++ allocation, не все C/OS
allocator и не доказательство полноты конкурентного GC. Все стадии открыты.

Проверено v0.444: Linux minimal 879/879; целевые minimal/AGG/NO_OS_PORTS на Linux/
heap-global ASan+UBSan — по 5/5 после восстановления dependency-cache. Первый
ASan-прогон обнаружил старый function.cpp.o с прежней раскладкой CallScope;
перегенерация зависимостей пересобрала его, повтор прошёл. Применены средства
восстановления права запуска; stack ASan/LeakSanitizer отключены. Все стадии открыты.

## v0.445 — регистр super.__proto__ в старых SWF

В загруженных SWF5–6 варианты регистра super.__proto__ возвращают прототип
выбранного предка, как каноническое имя. Поиск переопределения тоже учитывает
версию: собственный getter с другим регистром находится до intrinsic-проекции.
SWF7–8 сохраняют точный регистр, NUL-суффикс отличает имя. Основание —
закреплённые super_object.rs proto/base_proto и регистрозависимая property map
script_object.rs. Обычная raw-модель __proto__ и все мутации ещё не завершены.

object_super_prototype_case воспроизвёл прежнюю выдачу прототипа receiver для
uppercase. Проверены canonical/upper/mixed/NUL в SWF5–8, перенос через очередь с
GC и топологией скопированного receiver, canonical/upper getter, исходный this
и число callback. jit_object_super_prototype_case повторяет GetMember в трёх
режимах с настоящими нативными входами. Полная рефлексия/прототипы и все стадии
остаются открытыми.

Проверено v0.445: Linux minimal 881/881; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.446 — регистр raw-полей sortOn

Загруженный Array.sortOn учитывает Flash-регистр SWF4–6 для собственных данных,
данных virtual-дескриптора и нативных методов. LENGTH у Array выбирает native
length по тому же правилу. Обёртка метода использует сохранённое имя для
идентичности кэша. SWF7–8 различают регистр; унаследованные поля исключены,
getter/setter не вызываются. Host-сортировка прежняя. Основание — закреплённые
globals/array.rs sort_on_compare с get_local_stored и регистрозависимая
property map script_object.rs.

array_sort_on_case воспроизвёл отсутствие сортировки поля с другим регистром.
Проверены SWF4–8, обычные/virtual/унаследованные/Array-length/native-method поля,
NUL и Unicode, числовое/строковое сравнение, обычный/unique/indexed результат,
неизменность входа при indexed, отсутствие callback, сохранённое имя кэша
метода и GC. jit_array_sort_on_case повторяет матрицу с настоящими входами
CallMethod в трёх режимах. Полная сортировка, коллизии/бюджеты и все стадии открыты.

Проверено v0.446: полный Linux minimal 883/883 и AGG 902/902; целевые
AGG/NO_OS_PORTS на Linux/heap-global ASan+UBSan — по 4/4. Полный AGG-профиль
включает шрифты/image codecs/SQLite/render jobs; это не отдельный Cairo-профиль
и не запуск на другой ОС. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Все стадии остаются открытыми.

## v0.447 — регистр C-вызова метода и корни владельца getter

C-мост вызова метода с counted-именем использует общий поиск SWF4–6 для
data/accessor и границы нативного метода. Собственная Function затеняет
наследуемый native-метод при эквивалентном старом регистре. Host и SWF7–8
остаются точными. Receiver/owner удерживаются, пока getter удаляет свойство/
отсоединяет прототип, а возвращённый метод запускает GC. Использован именованный
предел глубины. Прежний C-ответ для неподдерживаемого имени сохранён.

object_call_case воспроизвёл прежний отказ folded-вызова. Проверены host/SWF4–8,
exact/folded имена с NUL, собственные/наследуемые Function, virtual getter,
native-методы/затенение, исключения и счётчики. Script-методы запускают GC и
проверяют исходные receiver/owner; getter может удалить себя и отсоединить цепь.
Правило регистра следует закреплённым object.rs call_method/search_prototype и
script_object.rs property lookup; C-ошибки — контракт расширений. Полные входы
вызова, resolver и все стадии остаются открытыми.

Проверено v0.447: полный Linux minimal 884/884; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.448 — снимок C-аргументов до getter

C-мост вызова метода копирует значения аргументов в traced-вектор и удерживает
receiver до разрешения getter и очистки выхода. Getter с GC может менять
исходный массив аргументов, сохраняя подготовленный вызов; выход может совпадать
с аргументом. Управляемые string/Object/metatable удерживаются, внешние байты
строк принадлежат вызывающему коду. Общий снимок покрывает super и trusted
native, заменяя дублирующее копирование/корни последнего. Обычные native-barrier
сохранены.

object_call_argument_snapshot воспроизвёл сборку циклического аргумента внутри
getter. Проверены host/SWF4–8, traced-строка с NUL, циклический Object, изменение
исходных аргументов, общий вход/выход, исключения getter/метода, счётчики, очистка
и освобождение после возврата. Это контракт времени жизни C API; клонирование/
внешние буферы, native capture, отказы памяти и все стадии остаются открытыми.

Проверено v0.448: полный Linux minimal 885/885; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.449 — корень capture активного native-вызова

C-мост вызова метода удерживает capture выбранной native-записи на всё время
callback. Trusted callback с GC может удалить/заменить свой метод либо
глобальную fallback-регистрацию, сохраняя активный граф userdata. Корень
снимается после вызова; замена не откатывается. Обычные публичные native-callback
сохраняют GC-barrier. collector_entry — внутренний признак trusted-моста, а не
публичное разрешение C-расширениям обходить защиту.

object_call_native_capture воспроизвёл сборку capture после самоудаления.
Проверены host/SWF4–8, собственное удаление/наследуемая замена/global fallback,
циклические capture и аргумент, traced NUL-строка, изменение исходных аргументов,
OK/исключение/обычная ошибка, GC в callback, удержание pin/pending exception и
освобождение после возврата/clear/unpin. Другие entry/finalizer/конкуренция и
все стадии остаются открытыми.

Проверено v0.449: полный Linux minimal 886/886; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 4/4. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.450 — Object.__resolve

Добавлен raw-поиск resolver отсутствующего загруженного свойства с регистром
по SWF, исходным this/counted-именем, пропуском примитивов и остановкой на Object.
Getter resolver не вызывается, virtual даёт данные. Проверены именованные/
числовые/родительские чтения, GC, ошибки, special-преобразования без resolver,
ограниченная native-рекурсия и настоящие JIT-входы.
[EN](OBJECT_RESOLVE.md) / [RU](OBJECT_RESOLVE.ru.md). Прямые C/super-вызовы, scope,
перенос/OOM и все стадии открыты.

Проверено v0.450: полный Linux minimal 890/890; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 6/6. Затем добавленные случаи приоритета метатаблицы
повторены во всех четырёх профилях. Обновлены dependency-index для новой private
раскладки контекста. Применены средства восстановления права запуска;
stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.451 — вызов метода, возвращённого resolver

Загруженные direct C missing-method и явные super-методы используют raw-поиск
resolver. Возвращённая Function исполняется с исходным this, снимком исходных
аргументов и без владельца свойства. C для отсутствующего/нефункционального
значения сохраняет UNSUPPORTED; AVM1 parent-call даёт undefined. Настоящие ошибки
resolver/метода, включая UNSUPPORTED, передаются и не маскируются как отсутствие.
Special-преобразования по-прежнему пропускают resolver; host и global native
fallback без resolver сохранены. Lua-style callable Object не является AVM1 Function.

object_resolve_call воспроизвёл отказ direct C-вызова; отдельный контроль
выявил различие super/nonfunction callable. Host/SWF4–8 проверяют обычный/
родительский receiver, C/AVM1 API, Function/scalar/Object/Lua callable/отсутствие/
исключение/UNSUPPORTED, counted-имя, циклический/NUL аргумент, самоудаление,
отсоединение прототипа, изменение исходных аргументов, GC и очистку состояния.
jit_object_resolve_call повторяет CallMethod в трёх режимах с настоящими
native-входами, проверяя отсутствие повторного callback при ошибке. Scope,
перенос/OOM и все стадии остаются открытыми.

Проверено v0.451: полный Linux minimal 892/892; AGG, NO_OS_PORTS на Linux и
heap/global ASan+UBSan — по 5/5. Применены средства восстановления права
запуска; stack ASan/LeakSanitizer отключены. Все стадии и другие ОС открыты.

## v0.452 — публикация результата resolver при OOM

Resolver вызывается с частным результатом; наружу он публикуется только при OK.
Отказ копирования возвращённого view временного counted-имени оставляет выход
undefined, а не висячую строку. Исключение хранится в контексте; error-значения
не публикуются как успешный результат. Побочные эффекты регистрации/callback
не откатываются.

object_resolve_allocation воспроизвёл небезопасный выход при отказе new/new[].
Новые контексты перебирают границы до первого завершённого OK/исключения в
SWF4–8: named NUL/indexed, native Function/script body/native method resolver,
циклический capture как payload, очистка корней/рекурсии/script depth, pending
payload, GC/повтор и удержание успешной строки. Это границы C++ allocation, не
все C/OS allocator и не доказательство конкурентности. Scope, перенос графов и
все стадии остаются открытыми.

Проверено v0.452: полный Linux minimal 893/893 и AGG 912/912; целевые
AGG/NO_OS_PORTS на Linux/heap-global ASan+UBSan — по 6/6. Первый полный AGG-прогон
обнаружил один старый jit_array_sort_on_case.o, созданный до private-изменения
контекста. Он пересобран, полный повтор 912 случаев прошёл. Применены средства
восстановления права запуска; stack ASan/LeakSanitizer отключены. Все стадии открыты.

## v0.453 — статусы resolver и исходные нативные интерфейсы

Наличие resolver отделено от статуса callback. Реальный [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) от
обработчика, возвращённого метода или отозванного RPC-интерфейса передаётся
без маскировки под отсутствие hook и без повторного вызова через fallback.
Проверки интерпретатора/JIT включают эти ошибки, super и счётчики вызовов.
object_resolve_rpc проверяет SWF6/8 в двух независимых runtime: исходный контекст
обслуживает исключительно поток брокера, callback и userdata остаются исходными;
проверяются циклический получатель, NUL/Unicode, исключения, ошибки, восстановление
и отзыв через один порт. В NO_OS_PORTS сетевой тест пропускается.

Пример C89 property_resolve возвращает исходную C-функцию для отсутствующего
sum, проверяет удержание захвата GC и печатает sum = 42. README на двух языках
описывают сборку/запуск и ссылаются на примеры RPC между процессами, потоками и
компьютерами LAN. Полная распределённая семантика графов и разрешение переменных
не заявлены. Все девять стадий и приёмка на других ОС остаются открытыми.

Проверка v0.453: Linux minimal 895/895; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 7/7 целевых проверок. Сетевой сценарий
пропущен в NO_OS_PORTS. Применены launchers для прав исполнения; stack ASan
и LeakSanitizer отключены. Другие ОС и все стадии остаются открытыми.

## v0.454 — обычные глобальные имена не вызывают resolver отсутствующего свойства

В загруженном SWF поиск обычной переменной проверяет наличие глобального
свойства до чтения. Отсутствующее имя не запускает __resolve; явное чтение
глобального члена сохраняет обработчик. Нативный fallback сохранён, наследуемый
undefined считается существующей привязкой и подавляет его. Host-путь сохранён.
Эталон: Ruffle scope.rs resolve, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
object_resolve_global_scope сначала воспроизвёл лишний callback; проверяет SWF4–8,
lookup/GetVariable/CallFunction, явное чтение, нативный fallback, наследуемый
undefined и три режима исполнения с реальными native entries. Полная
совместимость областей видимости и завершение стадий не заявлены.

Проверка v0.454: Linux minimal 896/896; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.455 — глобальный undefined сохраняет идентичность привязки

C API чтения глобального имени запоминает наличие свойства до вызова getter.
Собственный/наследуемый undefined и удаление свойства внутри getter подавляют
подмену зарегистрированной C-функцией в этом чтении. Следующее чтение после
удаления уже может вернуть нативную функцию. Учитывается версионный регистр
имён вместо точной проверки только собственной таблицы данных. Явный C API
сохраняет __resolve. global_undefined_binding воспроизвёл ошибку и проверяет
host/SWF4–8, данные, getters, наследование, удаление и регистр. Полная
совместимость глобальных имён и завершение стадий не заявлены.

Проверка v0.455: Linux minimal 897/897; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 3/3. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.456 — версионный регистр нативных глобальных имён

Чтение зарегистрированных нативных функций, fallback обычных переменных
и прямые C-вызовы используют общий выбор имени: SWF4–6 учитывают UTF-16-регистр,
host/SWF7–8 требуют точного совпадения. Обёртки используют сохранённое имя;
варианты написания возвращают один Function. global_native_case воспроизвёл
ошибку и проверяет ASCII, Unicode/NUL, host/SWF4–8, lookup/call, счётчики и
CallFunction в трёх режимах JIT/интерпретатора. Политика коллизий при регистрации
остаётся отдельной задачей; стадии не закрыты.

Проверка v0.456: Linux minimal 898/898; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 3/3. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.457 — снимок аргументов прямого глобального вызова

[fvm_call_bytes](../include/flashvm/value.h#L98) копирует аргументы в вектор с GC-корнем до очистки результата
и поиска Function. Результат может совпадать с аргументом; изменение исходного
массива callback не меняет активный вызов. Внешние строковые буферы по-прежнему
должны жить до конца вызова. Обычный GC-барьер нативных callbacks сохранён.
global_call_arguments сначала воспроизвёл порчу аргумента; проверяет host/SWF4–8,
C/script/getter/resolver, имена и строки с NUL, незакреплённые циклические объекты,
совпадение результата с аргументом, изменение массива, GC, ошибки/исключения,
очистку корней и освобождение. Host пропускает __resolve. Полная безопасность
C embedding и одновременного доступа к контексту не заявлена.

Проверка v0.457: Linux minimal 899/899; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 3/3. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.458 — замена нативной регистрации без лишнего корня захвата

Обновление нативной функции сохраняет выбранное имя в SWF4–6. Кеш обёртки
сбрасывается при изменении callback, userdata, захвата или collector-entry;
повторная идентичная регистрация сохраняет Function. Явно сохранённая старая
Function удерживает исходный callback и захват, ненужный кеш больше не удерживает
старый граф. Копирование имени выполняется до публикации: OOM сохраняет старую
регистрацию и кеш. В host/SWF7–8 варианты регистра остаются разными именами.
global_native_replacement воспроизвёл лишний корень; проверяет циклы, реальный GC,
версии, сохранённые Function и повторную регистрацию. Дополнительный allocation
тест перебирает new/new[] до первого успеха, проверяет неизменность при OOM,
повторную попытку и освобождение. Полная проверка аллокаторов и конкурентной
регистрации не заявлена; все стадии открыты.

Проверка v0.458: Linux minimal 901/901; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.459 — политика collector-entry входит в идентичность обёртки

При повторном использовании кеша Function сравнивается и collector_entry.
Новые чтения после внутреннего изменения политики получают актуальную обёртку;
сохранённые Function сохраняют исходную политику. Обычный барьер публичных
нативных callbacks остаётся. native_wrapper_policy воспроизвёл ошибку и проверяет
host/SWF4–8, глобальные и методные обёртки, изменение политики в обе стороны,
сохранённые функции, реальный GC из доверенного тестового callback и очистку
корней. Публичного отключения барьера и полной конкурентной проверки не добавлено.

Проверка v0.459: Linux minimal 902/902; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.460 — корни аргументов конструктора до поиска глобального имени

[fvm_construct_bytes](../include/flashvm/value.h#L155) создаёт снимок аргументов с GC-корнем до поиска конструктора.
Getter/resolver может изменить исходный массив или вызвать GC до начала
построения Function; зарегистрированная C-фабрика также получает стабильный
снимок. Обычный нативный барьер сохранён. Выход остаётся указателем на Object;
контракт наложения выходного указателя на вход не добавлен.
global_construct_arguments воспроизвёл порчу после изменения массива фабрикой.
Проверены host/SWF4–8, C/script/getter/resolver, NUL в именах/строках, циклические
аргументы, изменение массива, GC, объектный результат, ошибки/исключения и
освобождение корней. Host пропускает __resolve. Полная совместимость
конструкторов, аллокаторов и конкуренции остаётся открытой.

Проверка v0.460: полные Linux minimal 903/903 и AGG 922/922; Linux-hosted
NO_OS_PORTS и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer
отключены; все стадии и приёмка на других ОС остаются открытыми.

## v0.461 — версионный выбор имени нативного конструктора

Поиск и замена нативной фабрики используют тот же выбор регистра, что свойства
и глобальные C-функции. В SWF4–6 другое написание обновляет прежнюю регистрацию;
host/SWF7–8 остаются точными. Копирование сохранённого имени выполняется до
публикации: OOM сохраняет старую фабрику и захват. native_constructor_case
воспроизвёл неподдерживаемый alias; проверяет ASCII/Unicode/NUL, host/SWF4–8,
число регистраций, результат и освобождение циклического захвата. Allocation
тест перебирает new/new[] до успеха, проверяет старую фабрику/корень при отказе,
повторную попытку и GC. Это реестр C embedding; завершение ActionNewObject,
registerClass, аллокаторов и конкурентного доступа не заявлено.

Проверка v0.461: Linux minimal 905/905. Полный AGG дал 923/924: timezone
не запустил бинарник из-за сброса права исполнения (PermissionError). После
восстановления права отдельный повтор дал 1/1; все 924 проверки прошли
в этих запусках. Linux-hosted NO_OS_PORTS и heap/global ASan+UBSan — по 4/4.
Stack ASan/LeakSanitizer отключены; все стадии и другие ОС остаются открытыми.

## v0.462 — снимок аргументов прямого конструктора Function

[fvm_function_construct](../include/flashvm/function.h#L69) удерживает незакреплённую Function и копирует аргументы
с GC-корнями до нативной фабрики или чтения prototype. Getter может собрать
мусор и изменить исходный массив до вызова конструктора. Внешние строковые
буферы остаются ответственностью вызывающего; нативный GC-барьер сохранён.
Контракт наложения выходного указателя на вход не добавлен.
function_construct_arguments воспроизвёл порчу и проверяет host/SWF4–8,
script/native, prototype getter с GC, NUL-строки, циклы, изменение массива,
объектный результат, ошибки/исключения и очистку корней. Нативная constructable
запись задаётся внутренним тестом по модели встроенных фабрик. Полная
совместимость конструкторов, аллокаторов, конкуренции и все стадии открыты.

Проверка v0.462: Linux minimal 906/906; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.463 — проверка результата прямой нативной фабрики

Результат фабрики инициализируется undefined. Прямой конструктор Function
публикует только непустой Object своего контекста. Нетронутый результат, скаляр,
пустой Object и объект чужого контекста дают ERROR с пустым выходом; реальные
ошибки/исключения сохраняются. Это согласовано с именованными фабриками.
function_construct_native_result воспроизвёл ошибку и проверяет host/SWF4–8,
формы результата, payload исключения, GC и очистку корней. Произвольные
некорректные адреса хоста не входят в эту проверку; стадии остаются открытыми.

Проверка v0.463: Linux minimal 907/907; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 3/3. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.464 — прямой Function-вызов использует удерживаемый снимок

[fvm_function_invoke](../include/flashvm/function.h#L65) передаёт callback копию аргументов вместо изменяемого
массива хоста. Результат очищается после копирования и может совпадать с любым
аргументом. Ошибки проверки/OOM очищают выход; внешние строки остаются
ответственностью вызывающего. Раскладка корневого кадра и GC-барьер сохранены.
function_invoke_arguments воспроизвёл порчу; проверяет host/SWF4–8, native/script
и внутренний доверенный callback, обоих получателей, оба наложения результата,
циклы/NUL, GC, удержание строк, ошибки/циклические исключения и освобождение.
Полная приёмка C embedding, аллокаторов, конкуренции и стадий остаётся открытой.

Проверка v0.464: Linux minimal 908/908; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. После удаления повторной очистки выхода
тест вызова выполнен ещё раз. Stack ASan/LeakSanitizer отключены; стадии открыты.

## v0.465 — скриптовый конструктор AVM1 игнорирует return

Загруженный скриптовый конструктор возвращает созданный this независимо от
возвращённого значения скрипта. Нативная фабрика сохраняет собственный Object;
host сохраняет прежнюю подстановку возвращённого объекта. Эталон —
FunctionObject::construct в Ruffle function.rs, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd.
function_constructor_return воспроизвёл подмену this. Реальный байткод host/SWF4–8
возвращает undefined/Number/Object/Function/Array/this; проверены побочные эффекты,
прямой и именованный C API, NewObject/NewMethod в трёх режимах с native entries,
GC. Два ранних теста аргументов ожидают правильный this, сохраняя проверки
аргументов/мутаций/GC. Полная классовая семантика, свойства идентичности
конструктора, нативные callable-конструкторы и все стадии остаются открытыми.

Первый полный прогон нашёл два старых boxing-теста с неверной подстановкой
return. Ожидания исправлены: TargetPath сохраняет положительную проверку реального
MovieClip, instanceof связывает prototype с созданным this. GC, исключения и
счётчики сохранены. Итог v0.465: Linux minimal 909/909; AGG, Linux-hosted
NO_OS_PORTS и heap/global ASan+UBSan — по 8/8. Stack ASan/LeakSanitizer
отключены; все стадии и приёмка на других ОС остаются открытыми.

## v0.466 — собственные свойства идентичности скриптового конструктора

До входа в тело созданный this получает __constructor__, а до SWF7 также
constructor со ссылкой на callee. Флаг только DONT_ENUM: свойства доступны
для записи/удаления. Принудительные stored-свойства обходят наследуемые setters
и watchers. Общий helper подготавливает карты свойств/флагов до публикации,
затем удаляет точные старые virtual-слоты; OOM сохраняет прежние метаданные.
Host сохранён. Эталон — define_constructor_props в закреплённом Ruffle function.rs.
function_constructor_identity воспроизвёл отсутствие own-свойств; проверяет
host/SWF4–8, видимость в теле, accessor traps, флаги, перечисление, запись/удаление,
GC, сериализацию графа без установки стандартной библиотеки в приёмнике,
NewObject в трёх режимах с native entries. MovieClip, нативные фабрики и стадии
остаются открытыми.

Проверка v0.466: Linux minimal 910/910; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 6/6. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.467 — идентичность зарегистрированного класса MovieClip

Attach, duplicate и размещение спрайта из SWF определяют метаданные конструктора
после initObject, до вызова класса. Версия исходного movie выбирает собственный
constructor до SWF7 независимо от текущего SWF/контекста приёмника. Старые
библиотеки используют текущую версию. В SWF7+ поле constructor из initObject
сохраняется; __constructor__ и legacy constructor принудительно скрываются.
Исключение сохраняет опубликованный клип с метаданными. Default/unregistered
классы не изменены. Эталон — post_instantiation/construct_on_existing в Ruffle.
Тест проверяет source5–8/ambient6,8, оба реестра, коллизии initObject, три пути
построения, исключения, GC и перенос без установки библиотеки в приёмнике.
Allocation-тест проверяет SWF4–8, data/accessor/native-method, OOM-сохранность
значений/флагов/кеша/захватов, повтор, отсутствие callbacks, NUL и освобождение
циклических корней. Полный lifecycle, все аллокаторы и конкуренция не заявлены.

Проверка v0.467: Linux minimal 912/912, AGG 931/931; Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 6/6. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.468 — сохранённый prototype скриптового конструктора

Загруженный конструктор читает собственное сохранённое значение prototype,
обходя getter, наследование и __resolve. Общий reader для instanceof/Implements
теперь учитывает сохранённые данные script-accessor. Нативный accessor без
таких данных даёт undefined. Host сохраняет обычный поиск; явное fn.prototype
продолжает вызывать getter. Эталон — prototype/get_data в Ruffle script_object.rs.
Новый тест воспроизвёл ошибочный прототип; проверяет host/SWF4–8, Object/primitive
данные accessor, throwing getters, native accessor, наследование, регистр,
resolver, GC, instanceof и NewObject в трёх режимах. Старый NewObject-тест
отличает явное чтение от построения, сохраняя аргументы/GC/исключения. Это
уточняет прежние предположения о prototype getter; снимки аргументов необходимы.
Prototype как native method, зарегистрированные клипы и полная совместимость
остаются открытыми.

Проверка v0.468: Linux minimal 913/913; AGG, Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 8/8. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.469 — сохранённый prototype зарегистрированного клипа по версии source movie

Для библиотеки SWF prototype класса читается как собственное сохранённое
значение, без getter, наследования или resolver. Object из данных accessor
сохраняет прототип; отсутствующие/примитивные данные дают пустую Object-цепочку
вместо отказа attach. Явная версия movie обеспечивает регистр source6/8 независимо
от текущего SWF. Host-библиотеки без версии сохраняют обычное чтение.
Новый тест воспроизвёл неверную цепочку. Source6/8 × ambient6/8 проверяют
Object/Number в accessor, native accessor, наследуемые/смешанные/отсутствующие
поля, прямой примитив, throwing traps, attach/duplicate/размещение, счётчики и GC.
Перенос в контекст противоположной версии без установки библиотеки сравнивает
новый клип с независимо клонированным существующим; C-native accessor исключён
из переноса. Script body attachment снимается по существующему контракту графа.
Отражение примитивного __proto__, native-method prototype, полный lifecycle
классов и другие ОС не сертифицированы; стадии остаются открытыми.

Проверка v0.469: Linux minimal 914/914, AGG 933/933; Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 6/6. Stack ASan/LeakSanitizer отключены;
все стадии и приёмка на других ОС остаются открытыми.

## v0.470 — собственный prototype в виде C-метода

Внутреннее чтение prototype теперь получает кэшированную Function-обёртку
собственного native method, не вызывая callback. Сохраняются source-version
регистр, приоритет data/accessor и отсутствие поиска по наследованию.
Общий helper возвращает [FVM_Result](../include/flashvm/flashvm.h#L40): ошибки создания обёртки передаются через
конструкторы, instanceof, ImplementsOp и регистрацию MovieClip.
Расширены function_constructor_stored_prototype (холодный и тёплый кэш,
SWF4–8/host, GC, instanceof, три режима NewObject) и
movieclip_constructor_stored_prototype (source6/8 × ambient6/8,
attach/duplicate/размещение, отсутствие callback). C-метод исключён из
переноса графа этим тестом; сетевые функции имеют отдельный контракт.
Extends сохраняет обычное чтение superclass.prototype согласно pinned Ruffle
activation.rs action_extends; это отличается от сырых чтений при construction.
Все стадии, примитивный __proto__ и приёмка на других ОС остаются открытыми.

Проверка v0.470: Linux minimal 914/914, AGG 933/933; Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены.

## v0.471 — создание loaded Number до преобразования аргумента

Loaded Number сначала выделяет receiver с собственным сохранённым prototype
и скрытыми constructor identities; только затем выполняет ToNumber. Getter
класса не запускается, primitive/отсутствующий prototype даёт пустую Object-цепь,
изменение класса из valueOf не меняет уже выбранную цепь. Host-путь сохраняет
прежнее обычное чтение после coercion. Общий внутренний allocator удерживает
класс и receiver и публикует только полностью подготовленный объект.
Основание: pinned Ruffle FunctionObject::construct/define_constructor_props и
number.rs constructor, который заполняет заранее созданный this.
number_constructor_stored_prototype проверяет SWF4–8, direct/named C API,
NewObject в трёх режимах, mutation из valueOf, native getter/method, primitive,
missing и mixed spelling. GC внутри factory блокируется native barrier;
реальный сборщик проверяется после возврата. native_instance_allocation
перебирает отказы new/new[] от холодной обёртки до metadata, проверяя null output,
чистые кадры, GC и повторную попытку. Это не сертификат всех native constructors.
String/Boolean и другие фабрики требуют отдельной сверки; все стадии открыты.

Проверка v0.471: Linux minimal 916/916, AGG 935/935; Linux-hosted NO_OS_PORTS
и heap/global ASan+UBSan — по 4/4. Stack ASan/LeakSanitizer отключены.

## v0.472 — loaded String and Boolean receiver preparation

Loaded String and Boolean now use the same prepared-receiver allocator as Number:
select own stored prototype data and define hidden constructor identities before
conversion. Primitive/missing prototype data yields an empty Object chain;
accessor callbacks and inherited lookup do not select the prototype. A String
argument's toString mutation cannot change the already selected receiver chain.
String keeps counted text and its own nonenumerable/nondelete length. Boolean
keeps versioned truth conversion without invoking object coercion callbacks.
Unloaded host constructor lookup order remains unchanged.

Reference: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
FunctionObject::construct/define_constructor_props and globals/string.rs and
boolean.rs constructors, which populate the preallocated receiver.
string_constructor_stored_prototype and boolean_constructor_stored_prototype
cover SWF4–8, direct/named C construction and NewObject/NewMethod in interpreter,
explicit native and automatic JIT modes, native accessor/method, stored script-accessor
data, inherited/missing/primitive/mixed-case prototype, skipped __resolve,
identities and post-return GC. String additionally checks
mutation during coercion, embedded NUL text/length, errors and exceptions.
native_instance_allocation now sweeps new/new[] failure boundaries through actual
Number/String/Boolean factories, including length publication, cold method
wrappers, null failure output, clean root/native scopes, collection and retry.
The native barrier still rejects collection inside native factories. Both new
regressions fail against the original v0.471 implementation (missing constructor
identity), and pass after the fix. This does not close constructor
compatibility or any roadmap stage.

### Русский

Загруженные String/Boolean выбирают собственный сохранённый prototype и получают
скрытые свойства конструктора до преобразования аргумента. Getter не вызывается;
примитивное/отсутствующее значение даёт пустую Object-цепочку. Изменение prototype
из toString не меняет выбранную цепь String; сохраняются NUL и собственный length.
Boolean сохраняет правила truth по версии SWF и не вызывает методы объекта.
Проверяются SWF4–8, C API, три режима NewObject/NewMethod, GC после возврата, ошибки и
исключения String и отказы выделения памяти трёх фабрик. Полная совместимость
конструкторов, другие ОС/CPU и все стадии остаются открытыми.

Validation / Проверка v0.472: complete Linux minimal 918/918 and AGG 937/937;
selected heap/global ASan+UBSan 4/4 (stack ASan and LeakSanitizer disabled).
Unicorn 2.1.4: all 26 synthetic encoder success/error checks passed for x86,
ARM/Thumb, ARM64, RV32 and RV64, including far branches and high helper addresses.
These are synthetic ABI checks, not full VM execution on foreign CPUs/OSes.

The final Boolean callback-trap refinement was rerun in minimal, AGG and sanitizer
profiles. Interrupted build outputs/executable permissions were repaired before
the final complete native runs; no behavior failures remain in those runs.

## v0.473 — loaded builtin construction and function-call semantics

Date/Error/Object/Array prepare stored prototypes and hidden constructor identities
before coercion/setters. Date snapshots its receiver before numeric projection;
Error setters see complete metadata. Loaded Object() is bare for nullish inputs;
Array() uses its retained builtin prototype; Error() initializes its caller and
returns undefined, and Error.toString returns raw message data. Appended callback
IDs preserve invocation distinctions across graph transfer. Internal Function
metadata arrays no longer execute public Array.prototype getters.

Nine targeted regressions reproduce failures on v0.472 and pass after the fixes;
seven factories now have cold-wrapper allocation-failure sweeps. See
[BUILTIN_CONSTRUCTION.md](BUILTIN_CONSTRUCTION.md) for tests, reference sources,
callback/GC policy, transfer fallback and remaining numeric-length/boxing limits.
Все стадии остаются открытыми: Date/Error/Object/Array получают prototype и
метаданные до callbacks; разделены обычные вызовы, исправлен raw message Error
и исключён getter Array.prototype из метаданных Function. Полная совместимость,
другие фабрики и реальные запуски других CPU/ОС не заявлены.

Validation v0.473: complete Linux minimal 926/926 and AGG 945/945; nine
selected heap/global ASan+UBSan checks passed (eight together, then the new
Function2 metadata check). Stack ASan and LeakSanitizer remain disabled.
Three scoped constant headers passed a strict C89 syntax probe. Executable
permissions were repaired before final native runs. A one-record malformed-wire
fixture explicitly strips new constructor edges; normal roundtrip tests keep them.
Foreign CPU/OS acceptance and all nine roadmap stages remain open.

Проверка v0.473: полные Linux minimal 926/926, AGG 945/945; девять выбранных
проверок heap/global ASan+UBSan прошли. Stack ASan/LeakSanitizer отключены.
Три новых заголовка констант прошли C89. Приёмка других CPU/ОС и все стадии открыты.

## v0.474 — primitive boxing uses current global constructors

Loaded Object(value), Function.call/apply share AVM1 current-global boxing instead
of stale native-registry/captured constructors. Missing/unusable bindings return
a bare Object for Object conversion and the global receiver for call/apply.
Inherited/case-selected bindings work; resolver lookup stays excluded; getter and
constructor errors/GC obey existing native/trusted barriers. Host behavior and
wire IDs stay unchanged, including transferred host adapters in loaded contexts
without destination standard installation. See [PRIMITIVE_GLOBAL_BOXING.md](PRIMITIVE_GLOBAL_BOXING.md).
Four drivers reproduce failures on v0.473: host/SWF4–8 matrices, three primitive
types/fourteen binding forms, all Action routes, real native entries, GC/exception
lifetime, transfer and new/new[] allocation sweeps. Full compatibility remains open.

Object(value) и call/apply в SWF используют текущие глобальные классы примитивов.
Fallback — новый bare Object либо глобальный this; __resolve пропускается.
Проверены подмены, getters, исключения, GC, перенос и OOM. Другие CPU/ОС и все
девять стадий остаются открытыми.

Validation v0.474: complete Linux minimal 930/930. AGG passed 935/949 in
the full run; 14 binaries could not launch after executable-mode resets.
After restoring their modes, the selected rerun passed 14/14: all 949 checks
have passing evidence across these runs. Four selected heap/global ASan+UBSan
checks passed, including the full allocation sweep. Stack ASan and LeakSanitizer
remain disabled. Generated logs and baseline binaries stay outside committed
source. No new foreign-OS/device or full foreign-CPU run is claimed.

Проверка v0.474: Linux minimal 930/930; все 949 AGG-проверок прошли в полном
прогоне и повторе 14 запусков после восстановления прав исполнения. Выбранные
heap/global ASan+UBSan — 4/4. Stack ASan/LeakSanitizer отключены; все стадии открыты.

### v0.475: Function class calls / вызовы класса Function

Loaded SWF 4–8 `Function()` returns a bare object without arguments and preserves
its first argument without coercion otherwise. Empty `new Function()` allocates
an ordinary object with the stored prototype and hidden constructor identities;
an object argument is returned unchanged. Unloaded host behavior is retained.
The separate call callback has stable graph identity 809. AVM1 NewObject/NewMethod retain native primitive results through an internal
value-returning boundary; the public object-only C API retains its strict contract. This does not close stages 1, 2, 5, 8 or 9.

В SWF 4–8 `Function()` возвращает объект без прототипа при отсутствии аргументов,
иначе первый аргумент без преобразований. `new Function()` без аргументов создаёт
обычный объект с сохранённым прототипом и скрытыми идентификаторами конструктора;
объект в первом аргументе возвращается неизменным. Поведение host API без SWF
сохранено. NewObject/NewMethod сохраняют примитивные результаты через внутренний путь
создания значений; публичный object-only C API сохраняет строгий контракт. Этапы остаются открытыми.

### v0.476: Function/Error property attributes / атрибуты свойств

SWF 4–8 Function.prototype.call/apply are nonenumerable and nondeletable but
writable. Error.prototype.toString has ordinary script-assignment flags and is
enumerable/deletable/writable. Installing before loading stages both flag tables
before any publication, preserving modified slots and rolling back on allocation
failure. Serialized prototypes retain their flags. Host defaults remain unchanged.
These focused checks do not close any roadmap stage.

В SWF 4–8 call/apply скрыты от перечисления и защищены от удаления, но доступны
для записи. Error.prototype.toString имеет обычные атрибуты присваивания скрипта.
При загрузке в host-контекст таблицы флагов сначала подготавливаются целиком;
пользовательские изменения сохраняются, отказ памяти ничего не публикует.
Перенос графа сохраняет атрибуты. Все этапы остаются открытыми.

Evidence / Проверки v0.476: minimal 935 and AGG 954 tests passed across full runs
and permission-repair retries (18/26 executable files respectively). Two focused
heap/global ASan+UBSan checks passed. Native platform and full compatibility
acceptance requirements remain unchanged.

### v0.477: prototype argument boxing / преобразование аргумента isPrototypeOf

Loaded isPrototypeOf now applies current-global primitive boxing before walking
the chain; nullish/unavailable results get a bare fallback. ToObject collapses
native constructor primitive results to undefined without changing NewObject's
value-returning contract. Host behavior and bounded cycle traversal remain.
Tests cover versioned globals, getters/errors, three execution modes and OOM.

В isPrototypeOf загруженного SWF добавлено преобразование примитивного аргумента
через текущий глобальный конструктор. ToObject принимает только объектный
результат; NewObject по-прежнему сохраняет примитив нативного конструктора.
Проверки затрагивают этапы 1/2/7/8, не закрывая их и не заменяя приёмку этапа 9.

### v0.478: watch callback boxing / преобразование callback в watch

Loaded watch converts its callback through current global boxing constructors,
after name conversion. A native factory may produce the actual installed
Function; unsuccessful conversion preserves an existing watcher. Handler roots,
GC after clearing the boxing global, graph transfer without standard installation,
name/factory ordering, exceptions and OOM replacement retries are checked.

В watch добавлено преобразование callback после преобразования имени.
Проверены обычные и ошибочные глобалы, сохранение прежнего наблюдателя,
GC/перенос обработчика и отказы памяти. Нативный GC-барьер при регистрации и
поведение host без SWF сохранены. Это работа этапов 1/2/6/7/8; этап 9 и остальные
стадии остаются открытыми до полной приёмки.

### v0.479: ASSetPropFlags conversion / преобразования ASSetPropFlags

Loaded target boxing and AVM1 set/clear conversion now precede property-list
conversion. Saturating UInt16 narrowing handles fractional/negative/NaN values;
only supported attribute bits 0–7 are accepted. Extended version flags remain
open. Target roots, conversion order/failures, atomic attribute publication and
OOM retries are tested. Host behavior and ABI/callback identities are unchanged.

В ASSetPropFlags исправлены преобразования объекта и числовых флагов, порядок
побочных эффектов и случаи дробных/отрицательных/NaN значений. Дополнительные
version-флаги ещё не реализованы. Проверки не закрывают ни один этап роадмапа.

### v0.480: ASSetPropFlags stored names / сохранённые имена атрибутов

Loaded ASSetPropFlags now resolves property-list names case-insensitively even
for SWF7–8, preserving the selected own slot's stored spelling in the staged
attribute map. Ordinary lookup and unloaded matching remain unchanged. Tests
cover data/method/accessor slots, NUL/Unicode/long names, repeated aliases,
enumeration, transfer into SWF8, three execution modes and OOM retries.

Исправлены сравнение имён и сохранение написания ключа в ASSetPropFlags.
Унаследованные свойства не изменяются; getters не вызываются. При переносе
в SWF8 атрибуты сохраняются. Конфликтующие собственные имена, version-флаги и
приёмка всех стадий остаются открытыми.

### v0.481: Math namespace metadata / атрибуты Math

Loaded methods/constants are readonly, nonenumerable and nondeletable. First-load
activation stages only unchanged canonical host fields, retaining custom state.
Already loaded contexts preserve explicit flag clearing on reload. Tests cover
26 fields, C/three AVM1/JIT write modes, enumeration/deletion, graph transport,
GC and first-load OOM retries. Existing numeric values/operations are unchanged.

Исправлены атрибуты Math и атомарная первая загрузка при предварительной установке
библиотеки. Пользовательские изменения и снятая после загрузки защита сохраняются.
Числовая точность и полная приёмка всех стадий остаются открытыми.

### v0.482 — сохранение встроенного состояния при reload / builtin reload state

Переход host → loaded теперь выполняется один раз: повторная загрузка SWF4–8
сохраняет изменённые флаги встроенных прототипов. Проверены все пары версий,
C/interpreter/explicit/auto JIT, GC и отказ выделения памяти с повтором.
See [BUILTIN_RELOAD.md](BUILTIN_RELOAD.md). This closes the repeated attribute
activation regression only; all nine stages and full movie lifecycle semantics
remain open.

### v0.483 — фиксированные Math constants / fixed Math constants

Loaded Math uses canonical binary64 constants, correcting LOG10E's libm-derived
one-bit rounding discrepancy. Host values and custom state remain preserved;
first-load updates are transactional under OOM. Проверены побайтовые значения,
C/AVM1/JIT, перенос графа, GC и повтор после отказа памяти.
See [MATH_ATTRIBUTES.md](MATH_ATTRIBUTES.md). Exact Math functions, cross-platform
runtime acceptance and all nine roadmap stages remain open.

### v0.484 — class attributes independent of prototypes

Number constants and Date.UTC now activate independently of customized public
prototypes. Сохраняются изменённые поля/флаги, primitive metatable и прототипы;
проверяются OOM, GC, перенос и C/AVM1/JIT. See
[STATIC_CLASS_ACTIVATION.md](STATIC_CLASS_ACTIVATION.md). This is a first-load
metadata fix, not completion of stage 1 or any other roadmap stage.

### v0.485 — independent String/Object class activation

First-load String.fromCharCode and Object.registerClass attributes now activate
independently of public prototype state. Подготовка атомарна при OOM, getter
prototype не вызывается, изменённые поля/флаги и reload сохраняются.
See [STATIC_CLASS_ACTIVATION.md](STATIC_CLASS_ACTIVATION.md). All nine stages
remain open; full customized prototype and platform acceptance is not claimed.

### v0.486 — independent prototype method attributes

Number/Date/String/Array now prepare each canonical native method's default
SWF attributes independently of optional prototype payload conversion.
Изменённый anchor-метод не мешает остальным: проверяются пользовательские
флаги/data/accessor, OOM, GC, перенос и C/AVM1/JIT. See
[PROTOTYPE_METHOD_ACTIVATION.md](PROTOTYPE_METHOD_ACTIVATION.md). Customized
payload/layout policy is preserved; all nine roadmap stages remain open.

### v0.487 — UInt16 ASSetPropFlags and version visibility

Реализованы UInt16-атрибуты и маски видимости SWF4–8 для обычных свойств,
getter, нативных методов, __resolve и super; запись снимает версионные биты,
перенос сохраняет их. Membership/enumeration и raw Array имеют отдельные правила.
See [PROPERTY_VERSION_FLAGS.md](PROPERTY_VERSION_FLAGS.md) for tests, embedding
contracts and old-reader limitations. The builtin version-annotation audit,
intrinsic field interactions and all nine roadmap stages remain open.


### v0.488 — exhaustive property attribute regression coverage

Добавлены полный перебор 65 536 UInt16-наборов атрибутов для SWF4–8,
проверки записи/readonly/удаления и повторных вызовов интерпретатора,
явного и автоматического JIT. See [PROPERTY_VERSION_FLAGS.md](PROPERTY_VERSION_FLAGS.md).
This strengthens regression evidence for stages 1/7/8; it adds no new player
feature and closes none of the nine stages. Foreign OS/architecture runtime
acceptance and the builtin version-annotation audit remain open.


### v0.489 — signed Array constructor length

Загруженные Array(number)/new Array(number) используют clamped Int32,
включая дробные/отрицательные значения, NaN и бесконечности. Знаковая длина
сохраняется в native storage и графовом переносе, читается без unsigned-cast;
проверены readonly, рост индексов, GC и OOM/retry. See
[ARRAY_SIGNED_LENGTH.md](ARRAY_SIGNED_LENGTH.md) for RU/EN contracts and limits.
Raw unsigned length assignment remains distinct. Negative native end/splice
operations, virtual length and all nine roadmap stages remain open.


### v0.490 — native Array operations on negative length

Native push/pop/shift/unshift теперь используют знаковую длину, raw данные,
рост до записи и последующее сокращение length с отдельными readonly/nondelete
правилами. Проверяются virtual data, отсутствие getter/setter/watch вызовов,
версионные флаги, GC/перенос, C/AVM1/JIT, OOM и бюджет. See the v0.490 section
of [ARRAY_NEGATIVE_ENDS.md](ARRAY_NEGATIVE_ENDS.md). Negative native splice,
signed overflow and all nine roadmap stages remain open.


### v0.491 — native negative splice and captured-bound routing

Native splice поддерживает отрицательную длину и знаковые ключи. Captured
negative bounds принудительно выбирают generic путь даже после coercion,
изменившего живую длину, или при unsigned raw storage. Проверяются readonly/
nondelete, virtual data, version bits, C/AVM1/JIT, GC/перенос, OOM и бюджет.
See [ARRAY_NEGATIVE_SPLICE.md](ARRAY_NEGATIVE_SPLICE.md). Signed overflow,
arbitrary/virtual native length and all nine roadmap stages remain open.


### v0.492 — retained numeric Array length

Loaded numeric length assignment retains fractional, signed, nonfinite and
out-of-Uint32 Numbers, with separate wrapping Int32 traversal/truncation bounds.
Сохраняются primitive metatable/GC-циклы и графовый перенос; readonly/nondelete,
version flags и методы, записывающие/не записывающие length, проверены отдельно.
See [ARRAY_NUMERIC_LENGTH.md](ARRAY_NUMERIC_LENGTH.md) for RU/EN contracts,
coverage and wire-reader compatibility. Nonnumeric/virtual length and all nine
roadmap stages remain open.


### v0.493 — budgeted fast native splice staging

Fast loaded splice now charges stored-index copying and inserted arguments,
sharing the AVM1 budget and worker cancellation scope. Проверены C/direct invoke/
интерпретатор/JIT, sparse/ignored tails, real worker stop, staged-source preservation,
GC и OOM/retry. See [ARRAY_SPLICE_BUDGET.md](ARRAY_SPLICE_BUDGET.md).
Argument-coercion side effects remain visible after a staging error. Generic
mutations retain their existing partial-write policy; all nine stages remain open.

### v0.512 — Undefined this for anonymous CallMethod

Implemented / реализовано: anonymous loaded CallMethod supplies an Undefined
script this cell, separates Function2 preloads from inherited caller cells,
and creates no super facade for an Undefined actual receiver. Named nested
calls restore their own receiver, then the suspended caller metadata.
Tests / проверки: anonymous_undefined_this, its roots/allocation/transport
variants, and corrected anonymous_call_super regressions; interpreter and native
JIT body bridges share the policy. See [RU/EN contract](ANONYMOUS_UNDEFINED_THIS.md).
Qualified string paths, native receiver coercion, complete AVM1 compatibility
and all nine roadmap stages remain open / остаются открытыми.

### v0.513 — qualified Undefined this paths

Implemented / реализовано: GetVariable, SetVariable and qualified Delete2 use
Undefined this instead of the embedding receiver when selecting a path target.
Tests / проверки: anonymous_this_paths (including transport and native JIT),
its roots and allocation variants. See [RU/EN contract](ANONYMOUS_THIS_PATHS.md).
Arbitrary this-cell assignment, native receiver coercion and all nine stages
remain open / остаются открытыми.

### v0.514 — NanoSVG static-dependency audit

Implemented / реализовано: ELF audit rejects upstream libnanosvg.so sonames as
well as the existing fvm_* aliases. Regression fixtures demonstrate the former
false pass and stale-evidence replacement. See [EN](PLATFORM_VALIDATION.md) /
[RU](PLATFORM_VALIDATION.ru.md). Foreign OS execution, transitive/dlopen audits
and all nine roadmap stages remain open / остаются открытыми.

### v0.515 — mutable typed this cells

Implemented / реализовано: exact SetVariable this in SWF5–8 replaces the active
typed cell; typed Function2 inheritance, GC roots for suspended values, owned
borrowed strings, With/Catch copies and shared Try/Finally state. Tests / проверки:
this_cell_assignment, roots/allocation/transport variants and this_cell_blocks.
See [RU/EN contract](THIS_CELL_ASSIGNMENT.md). Native receiver coercion, complete
scope/SWF semantics and all nine roadmap stages remain open / остаются открытыми.

### v0.516 — SWF5 With this keyword case

Implemented / реализовано: the innermost With scope overrides the local frame
hint for this keyword folding; local functions with With closures retain local
recognition. Tests / проверки: with_this_case (including transport and native JIT),
its roots and allocation variants. See [RU/EN contract](WITH_THIS_CASE.md).
Complete scope/SWF semantics and all nine stages remain open / остаются открытыми.

### v0.517 — exact Array index whitespace

Implemented / реализовано: vertical tab no longer classifies a literal Array
property as an index or triggers overflow truncation. Tests / проверки:
array_index_whitespace, array_index_whitespace_effects and corrected
array_named_index expectation. See [RU/EN contract](ARRAY_NAMED_INDEX.md).
Nonnumeric Array length and all nine stages remain open / остаются открытыми.

### v0.518 — exact root path keyword

Implemented / реализовано: only exact first-token _root bypasses ordinary
property lookup in loaded paths; mixed-case spellings retain shadowing,
getters and qualified call receivers. Tests / проверки: path_root_case and
its roots/allocation variants. See [RU/EN contract](PATH_ROOT_CASE.md).
General path compatibility and all nine stages remain open / остаются открытыми.

### v0.519 — fail on missing CTest command artifacts

Implemented / реализовано: native ELF discovery rejects missing/directory
command files inside the build tree even alongside valid artifacts; output
arguments remain optional. Tests / проверки: test_elf_dependencies includes
false-pass and stale-status regressions. See PLATFORM_VALIDATION.md and
PLATFORM_VALIDATION.ru.md. All nine stages remain open / остаются открытыми.

### v0.520 — apply respects deleted Array length

Implemented / реализовано: loaded Function.apply uses zero arguments when an
Array list lacks own length data, instead of reviving cached native length.
Tests / проверки: function_apply_deleted_length and roots/allocation variants.
See FUNCTION_APPLY.md and FUNCTION_APPLY.ru.md. Other native Array deletion
semantics and all nine stages remain open / остаются открытыми.

### v0.521 — live own Array length in native methods

Implemented / реализовано: deleted/recreated length is read as live own data;
native writes recreate ordinary length, preserve readonly policy and truncate
only old numeric data. Incoming moves are rooted across coercion. Tests /
проверки: array_deleted_length plus roots, flags and allocation variants.
See [RU/EN contract](ARRAY_LENGTH_STORAGE.md). Indexed assignments, sortOn
argument-list lengths and all nine stages remain open / остаются открытыми.

### v0.522 — live sortOn field and option list lengths

Implemented / реализовано: sortOn captures own signed field-list length once,
converts names before option-list length, and ignores mismatching options.
Tests / проверки: array_sort_on_live_lengths plus roots/allocation variants.
See [RU/EN contract](SORT_ON_LIVE_LENGTHS.md). General indexed writes and all
nine stages remain open / остаются открытыми.

### v0.523 — loaded sortOn beyond 64 fields

Implemented / реализовано: loaded sortOn grows dynamic field/option preparation
instead of rejecting counts above 64; each compared field charges shared work.
Tests / проверки: array_sort_on_many_fields plus roots/allocation variants.
See [RU/EN contract](SORT_ON_MANY_FIELDS.md). General Array/SWF compatibility
and all nine stages remain open / остаются открытыми.

### v0.524 — live length before general Array indexed writes

Implemented / реализовано: general indexed writes convert live own length once,
apply raw growth before own flags/setters and preserve setter length mutations.
Detached input graphs stay rooted across coercion. Tests / проверки:
array_index_live_length plus roots/allocation variants.
See [RU/EN contract](ARRAY_INDEX_LIVE_LENGTH.md). Remaining Array/SWF semantics,
real foreign-platform acceptance and all nine stages remain open / остаются открытыми.

### v0.525 — native method-valued own Array length

Implemented / реализовано: raw own native method slots resolve to stable Function
data for length coercion, preserving case policy, GC roots and wrapper errors.
Tests / проверки: array_method_length plus roots/allocation variants, including
graph roundtrips and C/interpreter/JIT routes.
See [RU/EN contract](ARRAY_METHOD_LENGTH.md). General Array/SWF compatibility,
real foreign-platform acceptance and all nine stages remain open / остаются открытыми.

### v0.526 — parallel Android executable staging

Android CTest/ADB invocations upload unique binaries while retaining shared
fixture working directories; cleanup preserves primary failures and removes only
the invocation's binary. Девять host transport tests включают одновременные
запуски одного target, ошибки загрузки/очистки и timeout. See PLATFORM_VALIDATION.md
and PLATFORM_VALIDATION.ru.md. This is stage 9 tooling evidence; real Android
execution and all nine roadmap stages remain open / остаются открытыми.

### v0.527 — typed native Array length

Loaded native length retains all valid value types; AVM1 conversion controls
truncation independently of stored data. GC and graph transport retain typed
cycles/metadata with malformed-reference validation. Tests: array_typed_length,
roots, allocation and wire variants. См. [RU/EN contract](ARRAY_TYPED_LENGTH.md).
Deleted/recreated/virtual length assignment, real foreign OS acceptance and all
nine roadmap stages remain open / остаются открытыми.

### v0.528 — recreated/virtual Array length assignment

Exact loaded length writes prepare coercion/truncation before descriptor flags
and virtual dispatch. Loaded addProperty permits length and preserves its typed
data; descriptor re-resolution avoids double coercion after native reinstatement.
Tests: array_length_reassignment, roots/allocation variants. См.
[RU/EN contract](ARRAY_LENGTH_REASSIGNMENT.md). Original native-descriptor
mutation, complete compatibility and all nine stages remain open.

### v0.529 — native length reentry and obsolete GC edges

Exact native writes select the descriptor after conversion; deleting length
releases obsolete cached Object/String/metatable edges. Tests: native reentry,
cache GC/alias/shared-edge controls, existing typed/recreated suites. Linux
minimal: 1095 cases; nine targeted ASan/UBSan cases. См.
[RU/EN contract](ARRAY_LENGTH_CACHE_GC.md). Full compatibility and real foreign
OS acceptance remain open; no roadmap stage is closed.

### v0.530 — Linux validation checkpoint (documentation only)

Recorded v0.529 implementation evidence: minimal 1095; AGG/Cairo 1115 each;
26 fresh Unicorn cases per profile; direct ELF audits 1084/1104/1104 artifacts.
Nine targeted ASan/UBSan cases and 26 build-definition checks passed. См.
[RU/EN checkpoint and limits](LINUX_CHECKPOINT_V529.md). No executable feature
change. All nine completion stages remain open / остаются открытыми.

### v0.531 — build-selected automatic JIT

Implemented / реализовано: DEFS JIT seeds new contexts with automatic native
compilation and interpreter fallback; INTERPRETER retains the manual default.
Explicit options, independent contexts and destination graph policy remain
available. Tests / проверки: execution_build_default and execution_modes;
30 definition configurations. Native receiver frame metadata also fixes
BitmapData.loadBitmap resource selection; its GC/transport scenarios run in
three modes. See [RU/EN contract](JIT_BUILD_DEFAULT.md).
Full backend/platform acceptance and all nine stages remain open.


### v0.532 — automatic-JIT Linux validation checkpoint

Recorded / зафиксировано: clean v0.531 minimal-jit release-runner validation,
1097 CTest cases, 26 fresh Unicorn cases and 1086 direct ELF artifacts;
precommit minimal/JIT 1096 each, 30 definition cases and three ASan/UBSan cases.
See [RU/EN evidence and limits](LINUX_JIT_CHECKPOINT_V531.md). Documentation
only; real foreign OS/CPU acceptance and all nine stages remain open.


### v0.533 — VM fault diagnostics and preservation policies

Implemented / реализовано: independent NONE/BACKTRACE/ZIP/ZIP_CODE/DEBUGGER
fault actions and NATURAL/FORCE detail. Copied source/body reports, synchronous
live argument/local/register inspection, raw heap/class snapshots, CRC ZIPs,
active structured JIT bytes and original-report preservation across replay.
Tests: crash_c_api, crash_diagnostics, crash_allocation, crash_archive_host and
execution_mode_frame_replay; targeted ASan/UBSan. See
[RU/EN contracts and limits](CRASH_DIAGNOSTICS.md). First-chance VM statuses
are supported; asynchronous OS signals/SEH and native-PC unwinding remain open.
SWF/SWD source import is the next user-requested debugger work. No roadmap stage
is closed / все этапы остаются открытыми.


### v0.534 — source-aware debugger API and runnable tutorials

Implemented / реализовано: bounded matching SWF/SWD source import, copied
executing source lines and ZIP source modules, interpreter/JIT/native frame
classification, live frame/argument/local/register inspection at ordinary
breakpoints with fault action NONE. Added six C89 tutorials under
[examples/debugger](../examples/debugger/README.md), with every code line
commented, explicit errors and no assertions. Each tutorial documents purpose,
implementation, ownership and linked functions. The complete
[EN](DEBUG_API.md)/[RU](DEBUG_API.ru.md) reference describes all debugger APIs.
See [SWD contracts and limits](SWD_DEBUGGING.md). Tests include malformed/truncated
symbols, CWS, ambiguous bodies, atomic OOM retries, independent ZIP/source reads
and a tutorial/documentation contract check. All nine stages remain open;
OS signals/SEH, native-PC unwinding and foreign-platform acceptance remain open.

Validation / проверки v0.534: Linux x86-64 strict C90/C++98 Release minimal
and minimal-jit each passed 1110 CTest cases, including all six tutorials,
independent ZIP CRC/JSON/source reading and API/documentation coverage.
Five targeted ASan/UBSan tests passed: crash_diagnostics, crash_allocation,
execution_mode_frame_replay, swd_sources and swd_allocation (stack-use-after-return
on; LeakSanitizer off). Stale incremental objects were rebuilt and generated
ELF execute permissions repaired before the final complete passing runs.


### v0.535 — complete example catalog and API navigation

Documentation / документация: examples/README.md covers all 78 example folders,
base programs and standalone AS sources, with alphabetically sorted thematic
subgroups, linked contents and a flat alphabetic glossary. Each example links
to its own README, source files and demonstrated API guides/declarations.
Added missing README files and corrected obsolete example target names.
Executable C/C++, external-compiler AS and conceptual notes are distinguished.
Existing source paths and build targets are preserved. All nine stages remain open.

Validation / проверки: all 78 entries occur in the linked contents, descriptions
and glossary; subgroup/entry/glossary sorting checked. All 1870 relative file
and Markdown-anchor links resolve; every directly called public C API appears
in its example README. Existing debugger tutorial/API checker passed. No runtime
or build-target behavior changed.

### v0.536 — source-linked manuals and standing documentation rules

Documentation / документация: manuals and tutorial READMEs link referenced
functions, types, macros/defines and constants to actual source declarations
with current line anchors. A reusable refresh/check command maintains these
links after code movement. Standing API/example documentation and alphabetic
catalog requirements are recorded in AGENTS.md and the
[RU/EN maintenance guide](DOCUMENTATION_GUIDELINES.md). No runtime behavior
changed; all nine completion stages remain open.

Validation / проверки v0.536: 6170 relative Markdown/file links resolve,
including 3530 source-line links; symbol-labelled source links match the named
symbol on that line. The 78-entry catalog and debugger tutorial/API checks pass.
Repeated refresh is stable; simulated source-line movement updates links while
preserving headings and fenced examples. The existing isolated version-refresh
check passed. Runtime code was unchanged and runtime suites were not rerun.


### v0.537 — concise example descriptions in the catalog contents

Documentation / документация: all 78 example links in the structured contents
now have a short, smaller italic description on the same line. Alphabetical
groups, description anchors and the final flat glossary are preserved. The
format is recorded in the standing documentation rules. No runtime change;
all nine stages remain open.


### v0.538 — stage quality, ToggleQuality and explicit vector coverage

Implemented / реализовано: context-local quality API, BEST preference across
ToggleQuality, interpreter dispatch without operand changes and automatic-JIT
fallback. LOW controls solid/stroke/gradient/bitmap MovieClip edge coverage on
AGG/Cairo without global renderer state; standalone path APIs retain AA.
Added a line-commented C89 tutorial with full README/API links and catalog entry.
Tests: stage_quality and stage_quality_render; source dispatch is now 99/100.
Validation: minimal and minimal-JIT 1113/1113 each; AGG and Cairo 1132/1132
each; eight affected AGG+JIT tests passed ASAN/UBSAN. Generated ELF execute
bits lost by workspace synchronization were repaired before successful reruns.
See [RU/EN API contract](STAGE_QUALITY.md). Historic sampling grids, bitmap
quality differences, script quality properties, StopSounds and foreign-platform
acceptance remain open. No completion stage is closed.

### v0.539 — replaceable multimedia/device bridges and real FFmpeg backend

Implemented / реализовано: src/multimedia audio/video/camera/input/common,
C89 callback/session ABI, borrowed S16/RGBA buffers, explicit capture formats,
real FFmpeg demux/decode/drain/seek/convert/cleanup and pinned explicit static SDK.
Custom bridges remain available with NO_MEDIA. Native input adapters call Linux
evdev, Win32, Core Graphics and Android NDK functions behind target guards.
Added four annotated executable tutorials, source-linked guide and catalog.
Validation: MEDIA full suite 1120/1120; NO_MEDIA bridge tests 3/3; three
affected tests passed ASAN/UBSAN (upstream SDK not instrumented, LeakSanitizer
unavailable under the runner). Real-file tutorial fixture checks passed with
and without FFmpeg; static SDK recipe and source/catalog/API links verified.
Speech/TTS/STT is the next related bridge increment. Physical audio output,
SWF/AS multimedia integration, camera hardware and foreign OS runs remain open.
No roadmap stage is closed.

### v0.540 — explicit native TTS/STT providers and prepared-host tutorials

Implemented / реализовано: speech/tts and speech/stt; replaceable C89 voice and
recognition sessions with strict UTF-8, explicit microphone opening and retained
transcript query/copy. Real Windows SAPI TTS/dictation, Linux SSIP TTS and optional
static ALSA/Vosk capture/inference, macOS Speech Synthesis and optional
Speech/AVFoundation STT, Android JNI plus actual Java speech service helpers.
Explicit filesystem/abstract SSIP endpoints preserve binary names, embedded NULs
and exact native connect lengths on Linux/Android. Four documented tutorials cover executable TTS/STT and Android/macOS native app
preparation, real permissions, system services, ownership and cleanup.
Validation: MEDIA full suite 1128 passed, one explicitly skipped named Unix
endpoint test (1129 total; runner denies socket(AF_UNIX) before namespace choice).
NO_MEDIA speech checks: seven passed, same one skip. Five affected speech tests
passed ASAN/UBSAN; LeakSanitizer is unavailable in this runner. Real socketpair
I/O, binary names/lengths and intercepted connect arguments are tested separately
from an actual endpoint/daemon or audible-output run. Twenty Android JNI/Unix
adapter/example units passed C++98 source compilation with the official JNI
header; four Java sources compiled against Android API 35. Four optional Linux
ALSA/Vosk units compiled against ALSA 1.2.12/Vosk 0.3.45 headers; no complete Vosk
model/microphone run is claimed. Tutorial comments/API coverage, all 87 catalog
entries, source links and version metadata passed. Native Windows/macOS/Android
hardware acceptance remains open. No roadmap stage is closed.

### v0.541 — named and indexed AVM1 stage quality

Implemented / реализовано: MovieClip `_quality` and `_highquality` accessors,
GetProperty/SetProperty indices 16/19, case-insensitive counted labels and the
legacy numeric threshold policy. Named null/undefined handling remains distinct
from versioned action conversion (SWF4–6 zero, SWF7–8 NaN). Stable callback IDs
preserve accessors through graph transport; quality remains destination-context
state. Conversion errors leave stage state unchanged. Named native coercion
retains the existing prohibition on explicit GC; intrinsic action coercion is
protected by execution roots through real collection.

Validation: minimal full suite 1129 passed, one existing explicitly skipped Unix
endpoint test (1130 total). Three focused tests passed in the JIT profile and
three under ASAN/UBSAN with leak detection disabled for this runner. Tests cover
SWF4–8 in both requested execution modes, case/count boundaries, signed zero,
NaN/infinities/thresholds, coercion errors, GC and serialized native accessors.
The existing line-commented stage-quality example now demonstrates both named
properties; RU/EN contracts and compatibility inventory are updated. Pinned
Ruffle source is a semantic cross-check, not original Flash Player acceptance.
Historic quality sampling grids, StopSounds, native platform runs and the other
roadmap acceptance requirements remain open. No stage is closed.

### v0.544 — browser documentation viewer

Implemented / реализовано: root `view_docs.py`, engine in `doku_viewer/`,
loopback HTTP server, searchable Markdown navigation for project/docs/examples,
tables/fences/Unicode heading anchors, sanitized HTML, Pygments source lines,
`#L<number>` selection and scrolling, local CSS/JS and raw text views.
Pinned dependencies run on Python 2.7.9 and Python 3; no VM build is needed.
Validation: five actual HTTP/render/confinement tests pass separately on exact
CPython 2.7.9 and CPython 3.12. All 676 documentation/example/entry Markdown pages
render on 3.12. Links and all 87 example catalog entries pass. Chromium was
unavailable and its download failed, so browser screenshot/layout acceptance
and native Windows/macOS execution are not claimed. No roadmap stage is closed.

### v0.546 — context audio outputs, StopSounds and global stopping

Implemented / реализовано: six C89 channel APIs with copied host output tables,
active state, retryable failures, all-output stop attempts, native callback/GC
barriers and terminal context cleanup. AVM1 StopSounds and global stopAllSounds
use the same real host stop callbacks; stable global callback transport affects
only destination-context outputs. Added a fully commented PCM file-sink tutorial,
RU/EN detailed contracts and source links. Dispatch inventory now has 100/100
source references; this is not a sound/AVM1 completion certificate.

Validation: minimal full suite 1133 passed plus one existing explicitly skipped
named Unix endpoint test (1134 total). Three focused JIT and three ASAN/UBSAN
checks passed (LeakSanitizer disabled for this runner); four targeted MEDIA
checks passed, including real FFmpeg WAV decoding into registered PCM output
and StopSounds. Actual tutorial fwrite/fclose appended four bytes and reported
inactive/one stop. Atomic registration OOM, allocation-free stop/close, failed
output retry, callback reentry, teardown, function stack preservation and actual
serialized global callback isolation are tested. All 88 catalog entries and
109 multimedia/tutorial source files pass documentation checks. Full SWF sound
resources, streaming/mixing/envelopes and audible/native device acceptance remain
open. No roadmap stage is closed.

### v0.547 — optional native Linux ALSA playback output

Implemented / реализовано: explicit static ALSA SDK import, C89 open/close
factories, separate native open/write/start/drop/reprepare/close source adapters,
nonblocking complete-frame/error policy and independently host-owned provider
lifetime. Added an explicit-device, fully line-commented tone tutorial and RU/EN
API/preparation contracts. Default/foreign builds retain UNSUPPORTED factories.

Validation: five actual native tests passed on static ALSA 1.2.12, and the same
five passed ASAN/UBSAN on FlashVM (external SDK not instrumented, LSAN disabled).
Real built-in null PCM validates library operations; linker-wrapped short write,
EAGAIN, zero progress and start failures validate discard/retry state. The null
endpoint tutorial completed with EOF input, ELF has no dynamic libasound and
missing static SDK configuration is rejected. Default full suite 1136 passed,
one existing named Unix endpoint test explicitly skipped (1137 total). All 89
catalog entries and 115 bridge/tutorial files pass documentation checks.
Physical audible output, desktop server/plugin configurations, other OS playback
and full SWF sound integration remain open. No roadmap stage is closed.

### v0.548 — root SWF sound resource views and explicit PCM submission

Implemented / реализовано: five C89 APIs count/enumerate/find DefineSound root
resources in canonical loaded FWS/CWS, expose borrowed nominal metadata, decode
bounded native/little-endian 16-bit PCM ranges and submit callback-independent
copies to context audio channels. Exact PCM lengths, selected duplicate IDs,
version-3-format gating, sample/channel units, lifetime and temporary memory
limits are documented. This is explicit host resource output, not automatic
StartSound/SWF timeline playback. Added a full line-commented export tutorial.

Validation: minimal full suite 1141 passed and one existing named Unix endpoint
skip (1142 total). Four targeted native ALSA null-PCM pipeline tests and four
ASAN/UBSAN tests passed (external SDK/zlib not instrumented, LSAN disabled);
three targeted JIT-profile tests passed. Independent SWF1–8 mono/stereo fixtures,
CWS ownership, long headers, signed limits, capacity/range validation, malformed
PCM/duplicate IDs, allocation-free lookup/read, atomic OOM and successful reload
inside output callback are covered. Actual tutorial FWS/CWS raw export matched
expected bytes; unsupported/empty sounds created no output. All 90 catalog
entries and 117 bridge/tutorial files pass documentation checks.
Automatic StartSound, streaming, loops/envelopes/mixing, AS Sound, compressed and
eight-bit decoding, original player equivalence and foreign native platform
acceptance remain open. No roadmap stage is closed.
### v0.549 — SWF ADPCM resource decoding

Read/submit now decode format 1 with 2–5-bit codes, independent mono/stereo
state, 4096-frame blocks and signed saturation. Full declared-stream validation
precedes publication; no allocation or decoder cache is added. Metadata remains
nominal. Other compressed codecs, eight-bit PCM, automatic timeline playback,
mixing and native-platform acceptance remain open. No stage is closed.

ADPCM format 1 реализован для явного чтения/отправки ресурса. Проверяется весь
заявленный поток до записи; этапы роудмапа остаются открытыми.
Validation v0.549: minimal suite 1142 passed plus one existing named-Unix-socket
skip (1143 total). Three affected tests passed ASAN/UBSAN (LSAN disabled;
external SDK/zlib uninstrumented). Eight independently packed FLV/SWF fixtures
matched system FFmpeg byte-for-byte across widths 2–5 and mono/stereo, including
saturation, adaptive indices and block transitions. Truncation leaves output
unchanged; zero-length requests still reject truncated data. Catalog 90 entries
and source/anchor links checked. This is not original Flash Player acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call_bytes` | function | [include/flashvm/value.h:98](../include/flashvm/value.h#L98) |
| `FVM_CallScope` | class | [src/core/call_scope.hpp:8](../src/core/call_scope.hpp#L8) |
| `fvm_construct_bytes` | function | [include/flashvm/value.h:155](../include/flashvm/value.h#L155) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_function_construct` | function | [include/flashvm/function.h:69](../include/flashvm/function.h#L69) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../include/flashvm/function.h#L65) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
| `FVM_WatchWriteScope` | class | [src/core/watch_write_scope.hpp:8](../src/core/watch_write_scope.hpp#L8) |
<!-- source-links:end -->
