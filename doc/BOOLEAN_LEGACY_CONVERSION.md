# Boolean conversion / Преобразование Boolean — v0.192

Загруженные Boolean(arg) и new Boolean(arg) используют общие с AVM1 правила
truth conversion. В SWF4–6 строки преобразуются в числа: "0", "false" и "word"
ложны, "1" и "-2" истинны. В SWF7–8 любая непустая строка истинна. Объекты
истинны без вызова valueOf/toString, включая boxed false.

Boolean() без аргумента возвращает undefined в загруженном контексте;
new Boolean() хранит false. Незагруженный C API сохраняет прежнее поведение.
Общий helper используется и интерпретатором вместо дублирования правил.
boolean_legacy_conversion проверяет SWF4–8/host, строки, NaN/Infinity, null,
undefined, отсутствие object callback, traced GC и восстановление root/depth.

## English

Loaded Boolean(arg) and new Boolean(arg) share AVM1 truth conversion. SWF4–6
strings use numeric truth; SWF7–8 strings use nonempty truth. Objects are true
without invoking conversion methods, including boxed false. Missing function
arguments produce undefined; missing constructor arguments store false.
Unloaded host behavior is preserved. The interpreter uses the same helper.

Reference: Ruffle globals/boolean.rs function/constructor and value.rs as_bool,
commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Tests cover SWF4–8/host,
strings, NaN/Infinity, null/undefined, trap callbacks, traced GC and restored
state. Generic prototype layouts, function/clip version contexts and every
roadmap stage remain open.
