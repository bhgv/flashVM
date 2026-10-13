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
