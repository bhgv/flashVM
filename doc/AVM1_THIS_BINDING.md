# Implicit this precedence / Приоритет неявного this

v0.220: loaded SWF5–8 resolves recognized implicit `this` before ordinary local
bindings. A formal parameter or DefineLocal assignment named `this` cannot hide
the current receiver in ordinary calls or Function2 calls without this flags.
The scope/version casing rules from v0.219 still apply. Unloaded host calls and
SWF4 retain their previous local-name precedence.

В SWF5–8 распознаваемое неявное `this` разрешается раньше обычных локальных
записей. Параметр или DefineLocal с таким именем не скрывает получателя обычного
вызова или Function2 без флагов this. Правила регистра v0.219 сохраняются;
host-контекст и SWF4 сохраняют прежний приоритет локальных имён.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
`core/src/avm1/activation.rs` resolve returns this_cell before scope.resolve.
`avm1_this_binding` crosses ordinary/Function2 calls, formal/local/both/no
shadowing, SWF4–8 and unloaded host contexts. It pins the receiver and function
and collects before invocation. The test exercises bytecode lookup rather than
inspecting an internal binding map.

Inherited this-cell behavior under Function2 preload/suppression, mutable cells,
complete SWF4 keyword semantics, and full property-name case rules remain open.
This is a source-based conformance fixture, not independent complete Player
certification. The failing original this/AS2 probes remain outside the passing
17-case original SWF corpus. No ABI or wire-format change and no stage completion.

Наследуемая this-cell при preload/suppress Function2, изменяемые ячейки, полная
семантика SWF4 и регистр всех свойств остаются открытыми. Исходные this/AS2
по-прежнему не включены в 17 проходящих эталонных SWF. ABI и формат передачи
не меняются; завершение стадии не заявляется.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 378 and Cairo
Release 377 tests passed after execution-permission retries (3 and 1).
Ten selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan
(two sanitizer executable permission retries); stack/leak checks are disabled.
All 17 unchanged selected original SWF<=8 probes retained byte-exact traces,
zero exits and empty stderr. Non-Linux runtime verification remains open.
