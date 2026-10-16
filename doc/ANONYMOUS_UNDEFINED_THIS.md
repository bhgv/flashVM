# Anonymous CallMethod this / this анонимного CallMethod

v0.512 distinguishes an Undefined script this cell from a missing object pointer.
Loaded anonymous CallMethod (empty or Undefined method name) publishes Undefined
actual this. Regular functions and unflagged DefineFunction2 read Undefined;
PRELOAD_THIS receives Undefined. Function2 with either this flag inherits the
caller cell for named this reads, independently of the actual-this preload.
Super and PRELOAD_SUPER are Undefined when the actual receiver is Undefined.
Named calls and constructors publish their own receiver metadata, and all scopes
restore the caller metadata on success, exceptions and errors. The public script
frame receiver remains the embedding object; implicit_this is null for Undefined.
No public ABI or serialized graph format changes.

В v0.512 пустое или Undefined имя CallMethod передаёт Undefined в ячейку this
скриптовой функции. Обычная функция и DefineFunction2 без флагов читают Undefined;
PRELOAD_THIS также получает Undefined. Любой флаг this у Function2 сохраняет
наследование ячейки вызывающей функции для чтения имени this. Это не меняет
фактический аргумент preload. Super и PRELOAD_SUPER при Undefined получателе
также Undefined. Вложенные именные вызовы и конструкторы устанавливают собственный
контекст; состояние восстанавливается при обычном выходе, исключении и ошибке.
Публичный receiver кадра остаётся объектом моста, implicit_this равен нулю.
ABI и формат сериализации не изменены.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/activation.rs action_call_method; avm1/function.rs ActionFunction::exec,
load_this and load_super. This is a source comparison, not an original Flash oracle.
Сравнение с исходниками не заменяет проверку оригинальным Flash Player.

anonymous_undefined_this covers SWF4–8, Function2 flag combinations (SWF7–8),
empty/Undefined names, direct helper and interpreter/explicit/automatic JIT paths.
anonymous_undefined_this_roots checks nested named calls, inherited Undefined cells,
GC capture roots and success/throw/error restoration. Named Function2 calls
also separate an inherited Undefined local cell from object-valued this preload
and super based on their explicit receiver. Allocation sweeps check
retry with traced counted strings; transport checks destination caller cells.
The v0.509 anonymous_call_super fixtures retain storage/deletion/GC/transport
coverage but now expect Undefined rather than a fabricated superclass facade.

Remaining limits: qualified path strings such as this.foo and native receiver
coercion require separate compatibility work. No complete SWF or foreign-OS
acceptance is asserted. All nine roadmap stages remain open.
Ограничения: составные строки путей вроде this.foo и преобразования получателя
нативных функций требуют отдельной работы. Полная совместимость SWF и приёмка
других ОС не заявляются; все девять этапов роадмапа остаются открытыми.

Validation / Проверка: Linux x64 C++98 minimal 1051, AGG 1070 and Cairo 1071
cases passed. Execute permissions were repaired on 8/21/29 generated binaries;
minimal used failed-case reruns, AGG/Cairo repeated full suites. Cairo required
a second dependency-aware rebuild because its initial compiler dependency table
was empty and the first run mixed context layouts. Seven targeted ASan/UBSan
cases passed after relinking the first matrix executable; the strengthened nested
Function2 regression also passed separately. Twenty-six Unicorn branch/error
cases passed for x86, ARM, ARM64, RV32 and RV64; native x64 ran in host suites.
Five final fixture targets passed in minimal/AGG after the last test-only edits;
Cairo's final full suite includes those edits. Foreign OS acceptance remains open.
