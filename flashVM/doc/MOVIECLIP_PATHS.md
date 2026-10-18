# MovieClip path strings / Строковые пути MovieClip

v0.110 distinguishes default toString (`_level0.child`) from `_target`
(`/child`, root `/`). Root host names do not become path components. Names are
counted, preserving NUL and lone surrogates. Both builders inspect stored display
state without invoking replaceable `_name` getters and bound parent traversal at
1024 nodes. Root-level numbering comes from its stored depth; complete loaded
level lifecycle and removed MovieClip-reference semantics remain open.

Loaded AVM1 string coercion obtains the current display dot path directly and
ignores replacement toString methods, matching MovieClip-reference behavior.
Explicit method invocation and host C conversion still honor such overrides.
The current model uses live object identity; Flash's cached/stale/rebound
MovieClip reference rules are not implemented by this change.

Graph callback 1010 remains the default toString method. The separate _target
getter uses new stable callback ID 1005. When reading older graphs, a MovieClip
_target accessor carrying shared ID 1010 is restored as the slash getter;
other callbacks keep their registered identity. The graph wire version is
unchanged. Native callback roles remain transportable without standard globals.

Теперь toString и _target имеют разные представления, без имени корневого
клипа, заданного хостом. Встроенные NUL и суррогаты сохраняются. При чтении
старых графов прежний общий callback для _target восстанавливается как getter
слеш-пути. Полный жизненный цикл уровней и устаревших ссылок ещё не завершён.

movieclip_paths tests nested/root paths, counted names, old callback restoration,
transport into a context without standard installation, GC, AVM1 override bypass,
host override behavior and cyclic parent recovery. All 272 core AGG/Cairo tests
and the additional AGG runner test pass. Four focused tests pass in each portable
and ASan/UBSan profile with its existing restrictions. Ten original external SWF
probes continue to match exactly. Target-path resolution remains the next step.

## v0.111 — Counted target resolution and frame prefixes

The internal resolver supports absolute slash paths, relative slash/dot/colon
paths, parent `..` and `_parent`, initial `_root` (SWF 5+) and `_level0`.
After a slash, dots become part of the child name. Named display children have
priority over same-named dynamic properties; remaining object/prototype lookups
use the ordinary counted property bridge and actual receiver. Empty paths select
the start clip; missing or nonobject components return no target. SWF 4 keeps
its restricted delimiters; SWF 4–6 currently support ASCII case-insensitive paths
and explicitly reject non-ASCII paths pending the historic case table. SWF 7–8
paths preserve counted NUL and Unicode names. Other loaded levels remain open.

[GC:] Scoped roots retain original/root/current objects and the path across
script property callbacks. Root and token traversals are bounded. ActionCall
now splits a counted `clip-path:frame` operand, resolves its target, and invokes
that clip's frame actions; missing targets do nothing. Unseparated slash frame
syntax, full variable-path lookup and SetTarget execution remain open.

Пути теперь учитывают различия SWF 4–8 и приоритет display child над свойством.
Call другого клипа поддерживает форму `путь:кадр`, включая NUL в имени. Резолвер
сохраняет корни GC при скриптовом __index. Не-ASCII регистр старых версий,
остальные уровни и изменение target текущей активации ещё не завершены.

movieclip_resolve_paths checks delimiter/version rules, parent/root traversal,
child/property collisions, object aliases, missing components, NUL/Unicode,
legacy case gates and actual collection while walking a temporary script result.
avm1_frame_call additionally checks child/absolute/parent frame calls and counted
prefixes without moving either playhead. All 273 core tests in AGG/Cairo and the
additional AGG runner test pass; three focused tests pass in each portable and
restricted ASan/UBSan profile. No roadmap stage is closed by these results.

The v0.111 ASCII-only restriction is superseded in v0.378 by shared pinned
Flash unit folding: [EN](MOVIECLIP_UNICODE_PATHS.md) / [RU](MOVIECLIP_UNICODE_PATHS.ru.md).
Ограничение ASCII из v0.111 снято в v0.378; актуальные границы описаны по ссылкам.
