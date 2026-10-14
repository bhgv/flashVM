# Class and interface graph checkpoint / Классы и интерфейсы — v0.22

## English

AVM1 `Extends` (0x69) creates a new subclass prototype linked to the superclass
prototype without calling either constructor. `constructor` and `__constructor__`
refer to the superclass and are non-enumerable. `ImplementsOp` (0x2c) installs
validated interface constructor references on the implementing prototype.
`CastOp` (0x2b) preserves a matching object and returns null for a mismatch.

`instanceof` now follows both prototype inheritance and transitive interface
relationships. Cyclic interface declarations terminate through a visited work list.
The query is limited to 4,096 distinct prototype nodes; interface lists and operand
counts have the existing 65,536-entry bound. Invalid constructor operands and
non-integral/oversized interface counts report a result code rather than corrupting
the stack. Exact malformed-operand coercion for each Flash version remains open.

[GC:] Interface constructors are private retained graph edges, not visible AS
properties. Marking follows every edge. Class setup and membership queries protect
constructors, prototypes and pending interface lookups with stack root frames;
queries also work inside finalizers. Prototype property callbacks may execute code,
so interface lists are snapshotted and their referenced objects are rooted first.

Wire format 6 serializes these private edges using the graph identity table.
Readers retain support for versions 1–5. Interface references are range/type checked
before restoration; private watcher descriptors cannot acquire additional interface
edges. Public ABI 3 is unchanged. Existing file and network channel framing now
accepts wire 6.

`tests/avm1_inheritance.cpp` verifies inherited properties, superclass metadata,
absence of constructor side effects during Extends, class/interface membership,
transitive cycles, successful and failed casts, GC retention after interface globals
are removed, membership checks during finalization, and graph transfer into a
context without standard objects. `serialization_validation.c` now forms genuine
legacy records by omitting the wire-6 interface field; corruption/truncation tests
remain active. This is a functional fixture update, not a test-formatting pass.

Linux profiles: headless 169, AGG 179, Cairo 179, minimal 160, strict C89/C++98
with heap/global ASan and UBSan 166 checks. Executable-permission launch failures
are retried after restoring permissions. Sanitizer limitations remain unchanged:
stack ASan and LeakSanitizer disabled by this environment, container allocation
failure checks use UBSan only, SQLite is disabled in the sanitizer profile.

These operations supply class relationships needed by later `super` and SWF linkage
work. They do not implement `super` dispatch, timeline/display-list resources or
complete any roadmap stage. Opcode order/layout is taken from Adobe's
[SWF file-format specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

## Русский

Добавлены `Extends`, `ImplementsOp`, `CastOp`. Наследование создаёт новый прототип
подкласса без вызова конструкторов; ссылки `constructor` и `__constructor__`
указывают на суперкласс и не перечисляются. Объявленные интерфейсы хранятся в
приватном списке прототипа. Успешное приведение сохраняет объект, неуспешное
возвращает null.

`instanceof` учитывает цепочки прототипов и транзитивные интерфейсы. Циклы
останавливаются через список посещённых узлов. Ограничения: 4 096 прототипных узлов
на запрос и 65 536 элементов списка. Неправильные операнды и количества отвергаются;
полная сверка правил преобразования повреждённых операндов для версий Flash открыта.

[GC:] Интерфейсные конструкторы удерживаются и маркируются как обычные ссылки графа.
Временные кадры корней защищают конструируемые прототипы и рабочие списки запросов,
в том числе внутри финализатора. Список интерфейсов копируется и удерживается до
вызовов пользовательских аксессоров прототипа.

Формат графов повышен до 6; чтение 1–5 сохранено, публичный ABI остаётся 3.
Проверяется диапазон и тип интерфейсных ссылок. Дескрипторы watch не допускают
посторонних интерфейсных связей. Файловые и сетевые каналы принимают новую версию.

Перечень тестов и результаты приведены выше. Обновлённый тест старых форматов
удаляет новое поле при формировании legacy-пакетов, сохраняя проверки повреждений
и усечений. Это изменение логики теста, без его переоформления. Ограничения
санитайзеров и проверки только Linux указаны явно.

Операции подготавливают отношения классов для `super` и связывания с SWF.
Сам `super`, полноценная сцена и этапы плана целиком ещё не завершены.

## Loaded interface snapshots — v0.284 / Снимки интерфейсов

Loaded SWF7–8 ImplementsOp now captures prototype identities once, including an
installed empty list. Changes to constructor.prototype do not retarget those
edges. Getter-free reads, primitive/object count rules, GC and graph transport
are covered in [INTERFACE_SNAPSHOTS.md](INTERFACE_SNAPSHOTS.md). Host legacy
constructor-edge behavior is retained. Новый формат графа v9 сохраняет снимки;
обычные графы остаются в формате v8. Полная приёмка стадий ещё открыта.
