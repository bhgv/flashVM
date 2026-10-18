# Date function / Обычный вызов Date — v0.200

Загруженный Date() возвращает строку текущего локального времени без
преобразования аргументов. new Date(...) сохраняет отдельный native constructor.
Незагруженный C API сохраняет прежний constructor fallback. Функция включена
в staged functions и invocable class entry; allocation failures не публикуют
часть библиотеки. Новый стабильный callback ID 752 сохраняет функцию в графе,
ID 751 остаётся конструктором. Форматы графа и C API не менялись.

## English

Loaded Date() returns current local-time text without coercing operands;
new Date(...) remains native construction. Unloaded host calls retain their
historical constructor fallback. Staged function registration selects the class
invocable entry separately from its constructable entry. Captured class aliases
travel through the existing graph; callback 752 is the function and existing
751 remains construction. Graph/API format versions are unchanged.

Tests cover SWF4–8/host function vs construction, trap arguments, direct class
invocation, AVM1 CallFunction/NewObject, traced retained result strings and
GC/channel restored function/constructor paths. Reference: pinned Ruffle date.rs
function, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Exact localized text,
calendar/clock fidelity, remaining Date semantics and all stages remain open.
