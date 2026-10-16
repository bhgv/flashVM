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

## v0.485 — String and Object

RU: Независимая подготовка распространена на String.fromCharCode и
Object.registerClass. fromCharCode получает DONT_ENUM|DONT_DELETE и остаётся
доступным для записи; registerClass также получает READ_ONLY. Изменённый или
удалённый public prototype не препятствует обновлению нетронутых статических
методов. Изменённые поля класса и явные флаги сохраняются. Getter prototype
не выполняется; OOM не публикует частичные флаги.

EN: String static attributes now participate in the central precommit stage;
String prototype conversion no longer owns the class attribute transaction.
Object prepares/commits its class independently of its prototype as well.
The same three regressions cover all four classes, eight static fields, seven
prototype shapes, SWF4–8, C/interpreter/explicit/automatic JIT, graph transport,
GC, edited fields, first-load OOM and reload. The existing customized prototype
layout policy is unchanged. No public API, callback ID or graph format changes.

Additional primary sources: pinned
[String declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/string.rs)
and [Object declarations](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs).
All nine roadmap stages remain open.
