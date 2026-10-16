# Активация статических полей / Static class activation

## v0.484 — Number and Date

RU: Первая успешная загрузка SWF4–8 применяет стандартные атрибуты к
нетронутым константам Number и Date.UTC независимо от публичного prototype.
Удалённый prototype, null, число, обычный Object, accessor или изменение
опорного метода прототипа больше не мешают переходу атрибутов самого класса.
Действующие правила сохранения пользовательского прототипа не меняются.
Getter prototype не вызывается. Прежние значения, нестандартные атрибуты,
аксессоры и удалённые статические поля сохраняются. Константа Number с
пользовательской primitive metatable также считается изменённой.

EN: First-load class metadata is prepared independently of prototype-layout
eligibility. Untouched Number constants and Date.UTC become nonenumerable,
nondeletable and readonly even when the host edited/replaced the public
prototype. Class attribute tables commit without allocation alongside other
builtin activation. Allocation failure leaves the old class flags intact;
retry after GC completes activation. Reload preserves explicit live edits.
Existing conservative rules for customized prototype layouts remain unchanged.

Tests: static_activation_attributes (SWF4–8, seven prototype shapes, C and
interpreter/explicit/automatic JIT writes, deletion, GC, graph transport without
standard install in the destination, reload), static_activation_custom (flags,
values, accessors, deletion and primitive metadata), static_activation_allocation
(new/new[] failure sweep, getter suppression, GC and retry).

Primary metadata references: pinned Ruffle
[Number declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/number.rs)
and [Date declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/date.rs).
Host-before-load activation is FlashVM integration behavior, not a claim that
Flash Player exposes the same embedding lifecycle. All nine stages remain open.
