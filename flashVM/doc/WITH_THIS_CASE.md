# this folding in With / Регистр this внутри With

v0.516: a With scope overrides the local-activation hint when recognizing the
implicit this keyword. In SWF5 its folded spellings, including THIS and the pinned
Unicode lowercase equivalent, read the current typed cell even inside a function.
SWF6 remains insensitive; SWF7–8 remain sensitive. SWF4 and unloaded host controls
retain ordinary With shadowing. A new local function whose closure contains With
still has a local innermost scope and retains SWF5 case-sensitive recognition.

В v0.516 область With учитывается при распознавании this независимо от наличия
кадра локальной функции. В SWF5 THIS и соответствующие Unicode-варианты читают
текущую типизированную ячейку и обходят затеняющие свойства. В SWF6 сохраняется
нечувствительность, в SWF7–8 — чувствительность к регистру. SWF4 и host без SWF
сохраняют обычное затенение With. Новая локальная функция с With в замыкании
всё ещё имеет локальную верхнюю область и чувствительность SWF5.

The central lookup policy is shared by GetVariable and CallFunction. Catch under
With keeps the With scope class; its copied this cell remains independent of its
parent. Keyword reads do not evaluate shadow getters or bind their owner as this.
Non-keyword reads retain the existing getter/error/With-bound call behavior.
GetVariable и CallFunction используют общий обработчик. Catch внутри With сохраняет
класс этой области, но имеет собственную копию ячейки. Чтение ключевого слова
не вызывает getter затеняющего свойства. Остальные чтения сохраняют прежний путь.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/activation.rs resolve compares ScopeClass::Local in SWF5; action_with and
with_new_scope publish With, while action_try constructs Catch with the current
scope. This is source comparison, not an original Flash Player oracle.
Сравнение исходников не заменяет проверку оригинальным Flash Player.

with_this_case covers SWF4–8 and unloaded host, mixed/Unicode/suffix names,
local functions with With closures, pure function/scope/value graph transport,
interpreter and explicit/automatic native JIT. with_this_case_roots checks reads,
CallFunction and Catch, skipped getters, detached accessor/capture roots, GC,
throws and errors. with_this_case_allocation checks output atomicity and retry
for folded keywords and counted NUL suffixes. The main regression fails on v0.515.
Public ABI, graph format and generated instruction encodings are unchanged.
Full scope/SWF compatibility and all nine roadmap stages remain open.
Полная совместимость областей видимости/SWF и все девять этапов ещё открыты.

Validation on Linux x64, C++98: minimal 1062/1062, AGG 1081/1081 and
Cairo 1082/1082 passed, including reruns after restoring executable permissions
(15, 34 and 28 affected binaries respectively). Six targeted ASan/UBSan cases
passed. The strengthened same-function receiver-binding regression was then
rebuilt and passed separately in all four profiles. Direct DT_NEEDED audit of
1071 Cairo ELF artifacts passed; only libc, libgcc_s, libm and libstdc++ occur.
This does not certify transitive/dynamic dependencies or other operating systems.
Проверено на Linux x64: minimal 1062, AGG 1081, Cairo 1082 теста; после
восстановления прав запуска все повторы успешны. Шесть проверок ASan/UBSan
и усиленная проверка получателя вызова во всех четырёх профилях успешны.
Аудит прямых зависимостей 1071 ELF прошёл; другие ОС этим не подтверждены.
