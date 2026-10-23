# TextField scroll observers

Dimensions and layout policies now support [geometry range events](TEXT_GEOMETRY_EVENTS.md). Text/style-triggered events and input remain open.


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

## Transfer and legacy methods

The registry is an ordinary strong graph edge, including self-listener cycles.
Binary capture and channel transfer preserve registrations, source identity and
current offsets without replaying events. A pending stack-local delivery plan is
not serialized. Default fields that never register have no private registry edge.
No public C ABI struct or native Field wire bit is added by this series. Updated
readers are required for the new callback selectors 207/208.

Restoration upgrades only retired rootless native callback 205 when installed as
the TextField addListener/removeListener method, clearing its receiver method
cache. An independently saved old Function alias retains its unsupported body.
Custom methods, callback roots and userdata keep their ordinary capture contract;
raw host userdata is not made portable by this feature.

## Failures, execution and remaining scope

The 4096-entry limit rejects a new unique registration with [FVM_ERROR](../include/flashvm/flashvm.h#L43). Registration
staging and registry validation share a work budget. Recipient preparation precedes
publication; callback lookup and delivery consume the same event budget. Allocation
failure before publication preserves offsets. Lookup, VM exception, host C++
exception or work failure after publication retains the committed offsets and all
completed callback effects. Delivery stops at that error and has no automatic
retry or replay. Rewriting the same effective position does not replay a lost tail.
Conversion side effects also remain visible if subsequent preparation fails.

Callbacks run synchronously on the context owner thread. Reentrant setters retain
their final state. Recipient order is snapshotted, but each handler is looked up
live, so replacing a later handler affects the current delivery. The existing
native accessor GC barrier remains: collecting inside these callbacks is rejected;
collection between operations traces registered objects and reclaims unreachable
cycles. Worker coverage uses independent contexts, not concurrent access to one VM.
Native scroll query, path export and graph capture execute no observer code.

Geometry-wide notifications after text/style/dimension/maxscroll changes,
onChanged and user input, Selection/focus, HTML/mixed-style text and SWF text
display-list integration remain open. Tests use analytic layout and executable
AVM1 fixtures; they are not an original Flash Player binary oracle. Device and
foreign-platform acceptance is separate; no roadmap stage is closed.

## Validation

TextField explicit scroll observers v0.1104–v0.1117: addListener/removeListener use bounded graph-owned identities; effective explicit offset changes publish state before field-first onScroller and rooted snapshot listeners. Source arguments, duplicate/removal semantics, live handler replacement, nested writes and no-op suppression are verified. Binary/channel transfer retains registrations and collects unreachable cycles; only retired rootless method slots upgrade, while saved Function aliases and captured host overrides retain their bodies. SWF4/5 suppress these events; SWF6–8 real AVM1 setters agree across interpreter/manual/native JIT with actual native entry counts. Four independent OS workers check 128 epochs. Allocation sweeps and work limits distinguish prepublication rollback from committed partial delivery; VM/getter/C++ callback failures retain completed effects. Validation passes 163 selected native text/font/graph/tutorial checks, 17 ASan/UBSan checks plus explicit no-throw callback and no-fault allocation runs; LeakSanitizer is disabled. SDK-free C89 ABI/admission passes and three font tutorials explicitly skip. A renamed detached text component configures with bundled static font dependencies; its copied observer checker passes against the native executable, without a separate detached build. The C89 callback tutorial checks exact order and final offsets. Geometry-wide notifications, onChanged/input, HTML/mixed styles, SWF display-list text, physical devices/foreign platforms and all nine roadmap stages remain open. [Contract](TEXT_SCROLL_EVENTS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:25](../src/standard/text_listeners.hpp#L25) |
<!-- source-links:end -->
