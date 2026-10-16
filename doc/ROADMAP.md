# План завершения FlashVM / FlashVM completion roadmap

Принят пользователем 12 октября 2026 года. Основной ориентир дальнейшей
разработки. Исходная контрольная точка: v0.14. Добавление этого документа в
v0.15 не означает появления новых возможностей VM.

Цель: полная реализация Flash SWF ≤ 8 и соответствующих AS1/AS2 API,
запрошенные расширения, JIT и поддержка Windows/Linux/macOS/Android.
Требования: C89/C++98, без STL-контейнеров; платформенные адаптеры в ports/;
внешние библиотеки в external/, статическое подключение; читаемый код и
комментарии, тесты, документация на русском и английском.

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
фасадов обходятся итеративно с пределом; цикл возвращает FVM_ERROR.

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

[GC:] FVM_WatchWriteScope сохраняет граф исключения до очистки pending-состояния.
Явный корень защищает строки, объекты и циклы во время setter/GC. Деструктор
восстанавливает владение pending-значением до снятия корня и выполняется раньше
окружающего корня владельца поиска. Отказ выделения до приостановки сохраняет
исходное pending-исключение. При обычной записи вектор сохранения не выделяется.

object_watch_throw воспроизвёл ошибку v0.431. Проверены host/SWF4–8, именованные
данные, числовые слоты Object/Array, readonly, собственный/наследуемый setter,
успех/исключение setter и приоритет ошибок. Самоцикличный объект исключения
переживает настоящий GC внутри setter, где pending-состояние приостановлено.
Прежние тесты строк с NUL и JIT теперь ожидают запись undefined при loaded throw.
FVM_ERROR и OOM по-прежнему останавливают операцию; их эквивалентность ошибкам
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
host/SWF5–8, запись C-helper в исходный FVM_Value, Function, экземпляры, GC и
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

FVM_CallScope удерживает сохранённые/активные constructor, function и owner
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

Наличие resolver отделено от статуса callback. Реальный FVM_NOT_FOUND от
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
