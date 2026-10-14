# Function2 paired flags / Парные флаги Function2

v0.214 accepts simultaneous preload/suppress bits in DefineFunction2 decoding
and stored-function validation. Reserved bits and register bounds remain checked.
A paired this or super preload consumes its register but stores undefined;
a paired arguments preload still stores the actual arguments object. Combined
flags preserve preload ordering and subsequent register numbers. Named inherited
bindings and exact SWF-version scope behavior remain open; this patch does not
claim to fix those independent rules or register-parameter overlap policy.

DefineFunction2 и проверка сохранённой функции принимают одновременные
preload/suppress. Пары this/super занимают регистр со значением undefined;
пара arguments сохраняет объект аргументов в регистре. Зарезервированные биты
и границы регистров проверяются по-прежнему. Унаследованные именованные связи,
правила версий SWF и пересечение с регистрами параметров ещё открыты.

Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/function.rs, load_this/load_arguments/load_super. Tests extend
avm1_function2 for each pair and all three combined, explicit GC, and channel
transfer to a context without standard installation. avm1_super checks paired
super preload on an owned subclass method with an available superclass.
These source-based cases do not establish complete Function2/Flash Player
conformance; roadmap stage 2 stays open. Graph encoding and ABI are unchanged.

Тесты проверяют отдельные и совместные пары, GC и перенос через канал, а также
super у метода подкласса. Основание — указанные исходники эталона. Формат графов
и ABI не меняются; полная совместимость Function2 и этап 2 не завершены.

Validation / Проверка: Linux GCC/G++ 13.3, C90/C++98: full AGG 373 and
Cairo Release 372 tests passed (eleven/nine execution-permission retries).
Function2, super, functions and inheritance tests passed in NO_OS_PORTS and
heap/global ASan + UBSan. Stack ASan and LeakSanitizer remain disabled.
