# Object locale conversion / Object.toLocaleString — v0.206

В загруженных SWF Object.toLocaleString использует общий AVM1 string helper
для receiver вместо alias обычного Object.toString. Аргументы locale call
игнорируются. Переопределённый toString вызывается один раз без аргументов;
String-результат с NUL сохраняется, остальные результаты используют type label.
Ошибки передаются, recursion depth восстанавливается. Borrowed Object.toString
на callable/function receiver возвращает [type Function]. Host поведение сохранено.

## English

Loaded Object.toLocaleString now uses AVM1 receiver string coercion instead of
aliasing the ordinary Object string callback. It ignores supplied operands,
invokes a selected toString once with no operands, retains counted String data,
uses type-label fallback for other results and propagates errors. Borrowed
Object.toString recognizes callable/function receivers. Unloaded spelling remains.

New callback 627 identifies locale conversion; legacy IDs 611/612 still restore
the ordinary string callback. Tests cover SWF4–8/host, NUL strings, numeric/object/
undefined returns, errors, noncallable method data, function receiver, GC/graph
restored locale callbacks, recursion recovery and legacy callback identity.
Reference: pinned Ruffle object.rs to_locale_string/to_string and value.rs
string coercion, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Exact locale
formatting, raw __proto__, case/version availability and all stages remain open.
