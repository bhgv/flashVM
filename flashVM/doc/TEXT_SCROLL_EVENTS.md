# TextField scroll observers

This series extends the bounded [uniform scroll bridge](TEXT_SCROLL.md).
The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
describes field-first notification, listener registration and the source field
argument. The implemented event trigger here is an effective scroll/hscroll change
from an explicit offset setter. Input, geometry-wide automatic notifications,
HTML/mixed styled fields, full reference semantics and physical platforms remain
open. All nine roadmap stages remain open.

## Registration and graph ownership

TextField.addListener accepts one same-context object identity; duplicate adds
return true without reordering. Missing/non-object arguments return false without
coercion. removeListener returns whether an identity was removed. Foreign-context
values are invalid. SWF6–8 and unloaded host contexts are supported. Registration
is owner-thread only; no script callback runs during staging. Allocation/work
failure preserves live registrations. The bounded limit is 4096 listeners.

Registrations use a private nonenumerable, nondeletable managed graph edge to a
compact native Array. It is an implementation detail, not a public mutable list.
Ordinary tracing and graph serialization retain listeners and cycles; no raw
listener pointer is hidden inside the native Field resource. Receiver fonts,
fields and listener graphs are independently owned. The new native callback IDs
are stable. Existing host callbacks and captured roots must retain their identity.

## Explicit notification transaction

[FVM_TextFieldScrollPlan](../src/standard/text_listeners.hpp#L25) roots the source and copied registration order before the
setter publishes both offsets. Input conversion and fresh geometry happen first.
Preparation allocation/work errors preserve offsets. After publication the field
onScroller runs first, then snapshot listeners, each with one source-field argument.
Missing/noncallable handlers are ignored. First lookup/callback/work failure stops
delivery and propagates; committed state and callback side effects remain visible.
Nested writes remain authoritative. Registration mutations affect later events;
handler lookup is live for each recipient. Unchanged/clamped-same positions do
not notify. Query/path/capture keep their no-script contract. Existing native
accessor barriers still reject collection inside callbacks; ordinary GC resumes
after rooted delivery unwinds.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:25](../src/standard/text_listeners.hpp#L25) |
<!-- source-links:end -->
