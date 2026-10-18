# AVM1 display target scopes / Область target в AVM1

v0.112 implements SetTarget (0x8b) and SetTarget2 (0x20). Paths resolve from
the activation base clip; empty strings reset to that clip. Undefined resets
in SWF 4–6 and selects an invalid target in SWF 7–8. Missing targets select
invalid state without throwing. `this` remains the original function/frame
receiver. Clip variable lookup, assignment and top-level declarations use the
selected target; after invalid selection their clip fallback uses its display
root. Existing formal/local/lexical bindings retain their precedence.

[GC:] Base/current targets are context roots. An activation scope retains
suspended caller targets before publishing new state and restores them on all
exits. Fresh function/frame executions initialize their target separately;
nested bytecode blocks inherit the current state and restore their caller's
state on exit. A selected clip remains alive after removal of its parent edges
through script collection, then can be collected after activation exit.
Function-definition source-clip provenance and the complete scope/path model
remain open; the current base selection uses receiver/inherited host root state.

Next/Previous/Play/Stop/GotoFrame/GotoLabel/GotoFrame2 use display targets and
built-in controls, bypassing replaceable same-named script methods. GotoFrame
and GotoLabel stop playback. GotoFrame2 accepts numeric operands, counted labels,
unsigned decimal frame strings and scene offsets; invalid target uses its root
fallback. Missing labels and ordinary invalid-target controls are no-ops.
Label comparison uses complete UTF-16 strings, preventing NUL-prefix matches.
Other numeric string forms, path-prefixed goto operands, removed-clip semantics
and full player-version behavior remain unresolved.

SetTarget меняет область клипа, сохраняя `this`. Undefined различается в SWF
4–6 и 7–8; ошибочный путь не уничтожает базовый клип. Управление кадрами больше
не вызывает подменённые script methods. Тест gc.collect подтверждает жизнь
выбранного клипа после отсоединения, а после выхода он освобождается.
Происхождение функций из другого SWF/клипа и полный scope model ещё открыты.

avm1_target_scope covers SWF 4–8 selection/reset, fixed this, variable fallback,
control targeting/play state, override bypass, labels/numeric strings/NUL,
function target isolation, malformed operands, actual GC and target release.
All 274 core AGG/Cairo tests plus the optional AGG runner test pass (four Cairo
executables required restored execute bits and a successful rerun). Four focused
tests pass in each portable and restricted ASan/UBSan profile. Ten original
external SWF probes continue to complete and match exactly. All stages stay open.
