# String conversion / Преобразование String — v0.185

Загруженные String(...), new String(...) и concat arguments используют общий
AVM1 string coercion. SWF4 boolean даёт 1/0; undefined SWF4..6 — пустую строку;
нестроковый Object.toString даёт type label без host fallback. Отсутствующий
аргумент конструктора/функции по-прежнему даёт empty string. Concat сначала
сохраняет receiver, затем преобразует аргументы последовательно. Host правила
сохранены. Комментарий concat исправлен: это метод String, не Array.

string_legacy_conversion проверяет SWF4..8/host, function/constructor результаты,
undefined/boolean/Object, единственный callback, отсутствие аргументов, concat
нескольких значений, NUL output после traced GC и ошибки всех трёх путей.
Глубины/roots восстановлены. Boxed overrides, отдельные версии функций/клипов,
независимая полнота Flash Player и все стадии остаются открытыми.

## English

Loaded String calls, boxed construction and concat arguments share AVM1 string
coercion: SWF4 booleans use 1/0, SWF4..6 undefined is empty, nonstring Object
toString results use type labels without host fallback. Missing function/constructor
arguments remain empty. Concat captures its receiver before sequential argument
conversion. Host rules are preserved; the misleading Array concat comment is fixed.

Tests cover SWF4..8/host function/constructor results, undefined/boolean/Object,
exactly-once callbacks, missing arguments, concatenation, counted NUL output after
traced GC and errors on all three paths with restored root/depth state. Boxed
overrides, separate function/clip versions, independent full Flash Player validation
and all roadmap stages remain open. Reference: pinned String function/constructor/
concat coercion calls.
