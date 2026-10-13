# Пустые имена InitObject / Empty initializer names — v0.138

В загруженном SWF InitObject сначала преобразует имя по AVM1, затем игнорирует
пустую строку. Преобразование не пропускается: пользовательский toString всё
ещё вызывается. Значение и имя потребляются как обычно. В SWF4..6 undefined
как отсутствующее имя превращается в пустую строку и не создаёт свойства;
начиная со SWF7 создаётся имя «undefined». Это исправляет неверное ожидание
в предыдущих underflow/GC тестах, которые предполагали сохранение пустого ключа.

avm1_initializer_empty_names проверяет SWF4..8, явную пустую строку и callback
с результатом пустая строка (ровно один вызов). Незагруженный C-режим сохраняет
прежнее разрешение пустого ключа. Underflow и GC тесты уточнены. Общие правила
пустых имён SetMember/SetVariable ещё требуют отдельной сверки; исправление
касается InitObject, а не всех C property API.

Оригинальный pinned init_object_invalid SWF8 выполняется в --timeline 1 и даёт
все 16 эталонных байт без изменения SWF. Его счётчики/границы были исправлены
ранее; это расширение реального корпуса, не новый opcode. Проверяются 15
выбранных оригинальных проб. Новые add_property и init_object_order имеют
заголовок SWF15, остаются вне ≤8 и не включаются в успешный корпус.
[Источник](https://github.com/ruffle-rs/ruffle/tree/2372a4e148a67d68efd796f4477b3cddd4f880cd/tests/tests/swfs/avm1/init_object_invalid).
Все стадии роудмапа остаются открытыми.

## English

Loaded InitObject coerces names before ignoring an empty result. User toString
still runs once, and pair operands are consumed normally. Missing/undefined
names become empty in SWF4..6 and are ignored; SWF7+ spells undefined. This fixes
previous underflow/GC fixture expectations that incorrectly retained empty keys.
The new test covers SWF4..8 explicit and callback-produced empty names. Unloaded
host mode retains empty keys. General SetMember/SetVariable empty-name rules and
C API behavior remain separate open items.

Original pinned init_object_invalid SWF8 completes in --timeline 1 mode with
all 16 expected bytes, without editing inputs. Prior count/framing fixes already
make this newly selected probe pass; it expands the real corpus to fifteen.
New add_property and init_object_order fixtures are SWF15 and do not contribute
to supported oracle successes. All roadmap stages remain open.
