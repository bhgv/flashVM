# Implicit this name case / Регистр неявного имени this

v0.219: loaded SWF5 timeline lookup recognizes case variants of `this`, while
function local scope requires the exact spelling. SWF6 recognizes variants in
both scopes; SWF7–8 requires the exact spelling. Matching uses Flash UCS2
lowercase mappings, counted WTF-8 decoding and the entire name. Host contexts
and the existing SWF4 behavior remain unchanged.

В SWF5 timeline распознаёт варианты регистра `this`, а локальная область функции
требует точного написания. В SWF6 обе области игнорируют регистр; в SWF7–8
нужно точное `this`. Используется таблица UCS2-регистра Flash; встроенный NUL
или суффикс не превращает другое имя в ключевое слово. Host и SWF4 сохраняют
прежнее поведение.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
`core/src/avm1/activation.rs` resolve and `wstr/src/ops.rs` str_eq_ignore_case.
`avm1_this_case` checks timeline and ordinary function calls across SWF5–8
and unloaded host contexts: lowercase, uppercase, mixed case, Turkish U+0130,
suffixes and a counted name containing NUL. This source-based fixture does not
certify complete Flash Player conformance.

Local-binding precedence is covered separately in [AVM1_THIS_BINDING.md](AVM1_THIS_BINDING.md).
Inherited/mutable this-cell behavior,
Function2 this suppression/preload scope, full versioned property-name casing,
and SWF4 keyword availability remain open. Original this_swf5/6/7 probes still
fail later execution; AS2 class fixtures still fail loading. None is claimed as
a passing full reference test, and no roadmap stage is complete.

Приоритет локальных записей уточнён в [AVM1_THIS_BINDING.md](AVM1_THIS_BINDING.md).
Наследуемая/изменяемая this-cell, область this при
Function2 preload/suppress, регистр всех имён свойств и доступность ключевого
слова в SWF4 остаются открытыми. Исходные this_swf5/6/7 ещё не проходят целиком;
AS2-классы по-прежнему не загружаются. Завершение стадии не заявляется.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 377 and Cairo
Release 376 tests passed after execution-permission retries (13 and 1 respectively).
Nine selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan;
stack/leak checks are disabled. All 17 unchanged selected original SWF<=8 probes
retained byte-exact traces, zero exits and empty stderr. The separate failing
this/AS2 probes remain explicitly excluded from that passing corpus.
