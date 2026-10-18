# Date attributes / Атрибуты Date — v0.203

В загруженных SWF методы Date.prototype имеют DONT_ENUM | DONT_DELETE (3),
а Date.UTC — также READ_ONLY (7). Методы прототипа можно заменить значением,
но нельзя удалить без снятия защиты. Запись в UTC игнорируется; удаление
блокируется. Host defaults сохраняются: prototype methods 1, UTC 1.

При загрузке после установки подготовленные копии таблиц attributes входят
в общий атомарный commit. Штатные native methods с host flags 1 получают 3;
штатный UTC с host flags 1 получает 7. Отличающиеся от defaults пользовательские flags/descriptors и
заменённые значения сохраняются. Подготовка может отказать до изменений,
commit только меняет native state и swaps таблицы без allocation/callback.

## English

Loaded Date prototype methods are nonenumerable/nondeletable (3); Date.UTC is
also readonly (7). Prototype methods remain writable; UTC ignores writes and
cannot be deleted unless protection is cleared. Host defaults remain unchanged.

Loading after installation prepares attribute copies before any primitive
mutation. Shared installation now supplies only absent method attributes,
preserving explicitly staged flags instead of overwriting them. Unchanged native methods/default flags are upgraded, preserving
nondefault custom flags, descriptors and replaced values. Commit publishes native
state and swaps maps without allocation/callbacks. Wire/API formats remain.

Tests cover SWF4–8/both install orders, host defaults, customized flags, prototype
method writes/delete guards, UTC writes/deletes/calls, GC/channel retained flags
and allocation failures preserving old Date flags before retry. Reference: pinned
Ruffle date.rs PROTO_DECLS/OBJECT_DECLS, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd. Other builtin metadata, customized
native-layout migration and all roadmap stages remain open.
