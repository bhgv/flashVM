# Numeric attributes / Атрибуты Number и Boolean — v0.204

В загруженных SWF Number/Boolean prototype methods имеют flags 3
(DONT_ENUM | DONT_DELETE), сохраняя возможность записи. Пять штатных Number
constants имеют flags 7, блокирующие запись и удаление. Host defaults не меняются.

При установке после загрузки flags создаются сразу. При загрузке после установки
Number и Boolean подготавливают копии attributes до изменения любого прототипа.
Штатные методы с default flags 1 получают 3. Штатные Number constants без own
attributes получают 7, если значение не изменено. Nondefault flags, descriptors
и заменённые значения сохраняются. Commit использует только swaps и изменение
native state; OOM сохраняет все прежние прототипы/flags до успешного retry.

## English

Loaded Number/Boolean prototype methods use nonenumerable/nondelete flags 3 and
remain writable. The five default Number constants use readonly/nondelete flags
7. Installation after load creates flags directly; loading after install prepares
copies before any primitive mutation. Canonical method/default constant flags
are upgraded while nondefault flags, descriptors and changed data remain.
Commit swaps maps and native state without allocation/callbacks. Host defaults,
callback IDs and graph/API formats are unchanged.

Tests cover SWF4–8/both installation orders, host defaults, method writes/delete
guards, five constants, NaN write/delete guards, custom method flags, changed
MAX_VALUE/explicit MIN_VALUE flags, GC/channel preserved protection and OOM
retaining old numeric flags before retry. Reference: pinned Ruffle number.rs and
boolean.rs declarations, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Other builtin metadata, customized-layout migration and all stages remain open.
