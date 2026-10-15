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
