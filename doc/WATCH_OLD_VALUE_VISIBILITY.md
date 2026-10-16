# Watcher old-value visibility / Видимость oldValue watcher — v0.503

Loaded SWF4–8 watcher old-value lookup now skips storage hidden by the current
SWF version and continues through prototypes. The same gate applies to ordinary
properties, indexed slots, virtual-property retained data, lazy native methods
and the native Array length field. Lookup still avoids getter, __resolve and
metatable execution. Raw own membership, watcher matching and write policy are
unchanged. Host visibility has no version mask and retains its prior behavior.

При вычислении oldValue обработчика watch скрытое для текущей версии SWF
свойство теперь пропускается: поиск продолжается по прототипам. Getter при этом
не вызывается; для видимого виртуального свойства читаются сохранённые данные.
Правила наличия собственного свойства, выбора watcher и последующей записи
сохранены. Публичный ABI и формат сериализации не меняются.

watch_old_value_visibility reproduces the pre-fix failure and checks SWF4–8,
ordinary/indexed/counted-NUL/mixed-case names, hidden own data or virtuals,
visible inherited data or virtuals, absent fallback, retained-data writeback and
C/interpreter/explicit/automatic JIT routes. The existing watcher GC, allocation,
readonly and exception tests remain regression coverage.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
script_object.rs Watcher::call/call_watcher/get_local_stored and object.rs get_stored.
get_local_stored filters allow_swf_version before returning property data;
get_stored continues the prototype walk when a local value is absent.
This is source comparison, not an original Flash Player oracle. Complete SWF
compatibility, other callback rules and all roadmap stages remain open.

Validation / Проверка: Linux x64 C++98 full minimal 1018, AGG 1037 and
Cairo 1038 tests passed after execute-permission repair and rerun (1/2/4 test
binaries respectively; the Cairo reader was initially blocked by its writer
fixture). Five targeted ASan/UBSan tests passed. No original Flash runtime or
foreign operating-system acceptance is claimed.
