# Присваивание accessor / Accessor stored assignment — v0.137

После успешного setter обычное присваивание обновляет private data дескриптора
перед снятием корней native-аргумента. Строка копируется с counted length и
метатаблицей. Getter-функция остаётся действующей; исторические Array-методы
видят обновлённое data. Setter-Object без функции не вызывается, но data также
обновляется; plain getter после этого возвращает новое data вместо undefined.
Это исправляет более узкое поведение/описание v0.130, где запись полностью
игнорировалась. READ_ONLY остаётся проверкой обычного property API. Ошибка
setter не заменяет data; побочные эффекты уже выполненного callback не откатываются.

Allocation-free корень удерживает descriptor во время setter, даже если callback
удаляет свойство. object_accessor_assignment проверяет script getter/setter,
plain setter, NUL-строку, ссылку на владельца, readonly и UNSUPPORTED setter с
прежним data. Значения проходят traced-string GC и queue transfer с GC. Старый
object_add_property_objects теперь ожидает stored 81 после записи plain/mixed
дескриптора, сохраняя проверки callback-числа и flags.

Проверка не заявляет фактическую сборку внутри native setter: barrier остаётся.
Исторический set_data массивных мутаций, flags замены, thrown setter semantics,
методы/версии и все стадии роудмапа остаются открытыми.

## English

After a successful setter, ordinary assignment updates descriptor data before
native argument roots unwind, copying counted strings and preserving metatables.
A function getter still executes; loaded Array methods observe updated data.
A non-function setter has no executable callback but still updates stored data;
a plain getter subsequently returns it. This corrects v0.130's narrower ignored-
write behavior. Existing property API enforces READ_ONLY. Setter errors preserve
old data; already executed callback side effects are not rolled back.

An allocation-free root retains the descriptor through setter execution,
including self-detachment. object_accessor_assignment checks script/plain
setters, executable getter, counted NUL, owner cycles, readonly and an unsupported
setter with preserved data. Values survive traced GC and queue transfer/GC.
The prior plain descriptor test now expects stored 81 after assignment while
keeping callback and flag checks. No actual collection inside the native setter
barrier is claimed. Array mutation set_data, replacement flags, thrown-setter
semantics, method/version behavior and every roadmap stage remain open.
