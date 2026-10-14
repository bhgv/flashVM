# Borrowed String receiver / Receiver строковых методов — v0.181

Загруженные методы String используют общий AVM1 string coercion для receiver,
который не является boxed String. Вызывается toString; нестроковый результат
даёт [type Object]/[type Function] вместо host преобразования результата или
fallback valueOf. Типизированный boxed String и незагруженные правила сохранены.
Это распространяется на case, charAt/charCodeAt, substring/slice/substr,
indexOf/lastIndexOf, concat и split через общий receiver helper.

string_legacy_receiver проверяет SWF4..8, число/undefined/Object/NUL строку
из toString, единственный вызов, отсутствие valueOf fallback, ошибку callback,
charAt/charCodeAt/indexOf/concat/split, uppercase после traced GC и восстановление
root/depth state. Полные преобразования аргументов строковых методов, отдельные
версии клипов/функций, boxed overrides и все стадии роудмапа остаются открытыми.

## English

Loaded String methods use shared AVM1 string coercion for non-boxed receivers.
toString runs once; a nonstring result yields [type Object]/[type Function]
instead of host result coercion/valueOf fallback. Typed String receivers and
unloaded rules are preserved. The shared helper covers casing, character access,
substring/slice/substr, searches, concat and split.

Tests cover SWF4..8, number/undefined/Object/counted-NUL results, exactly-once
toString without valueOf fallback, callback errors, character/search/concat/split
behavior, uppercase after traced GC and restored root/depth state. Full argument
coercions, separate clip/function versions, boxed overrides and all stages remain
open. Reference: pinned AVM1 globals/string.rs receiver coerce_to_string calls.
