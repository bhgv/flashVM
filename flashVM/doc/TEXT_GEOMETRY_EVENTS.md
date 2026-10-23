# TextField geometry observer transactions

This extends [scroll observers](TEXT_SCROLL_EVENTS.md) for host-created uniform
TextFields. The [Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
describes notification for scroll property changes; the exact bounded geometry
and synchronous transaction below are implementation policy, not an original
Flash Player binary oracle. All nine roadmap stages remain open.

## Triggers and admission

Writes to _width, _height, multiline, wordWrap and autoSize compare effective
scroll, hscroll, maxscroll and maxhscroll before and after the candidate layout.
One change in this tuple delivers one event, even if only a maximum changes.
Unchanged tuples, parent _x/_y and textColor writes do not notify. A change only
to bottomScroll, line spacing or the clip rectangle is not a trigger here.
Explicit scroll/hscroll setters retain their existing contract.

GeometryNotification detects potential handlers through native property/method/
accessor descriptors on the prototype chain, using existing SWF6 folded-name and
version-visibility rules. A private registration edge or loaded __resolve descriptor
also admits the observed path. Detection runs no getter, resolver or callback.
Missing potential observers retain lazy measurement/admission. Even an empty
previously created registry or a noncallable descriptor admits measurement.
SWF4/5 keep the previous silent geometry behavior; SWF6–8 and unloaded host
contexts support geometry observers. Loaded dynamic resolvers remain subject to
the ordinary optional method dispatch contract.

## Prepare, publish and notify

First numeric conversion runs, possibly executing user code; preparation then
uses fresh source geometry. The candidate owns its text style copy and measured
layout independently, while the original dimensions/layout/offsets remain live.
An observed edit must admit both source and candidate layout; font/layout,
allocation and work errors propagate without publishing this edit. Successfully
built source caches and explicit conversion side effects may remain.
Unobserved edits retain the existing lazy behavior, so registering a handler can
make a formerly deferred layout error surface at a geometry setter.

A changed tuple prepares the rooted recipient snapshot before publication. A
successful commit publishes dimensions, layout policies, autoSize anchor/origin,
measured cache and clamped offsets together without allocation. The field
onScroller runs first, then saved listener identities in registration order, with
one actual source-field argument. Each handler is resolved live. Geometry and
maxima observed inside a handler already reflect the committed candidate.

First getter/resolver/callback/work failure stops delivery; it does not roll back
published geometry or completed effects. A host C++ exception follows the ordinary
method status mapping. No automatic retry/replay exists. Nested setters remain
authoritative: returning from the outer callback does not overwrite their state.
Recipient removal affects later events, while the current snapshot remains rooted.
One execution budget spans descriptor detection, preparation and dispatch.

## Ownership, GC and transfer

No public C ABI layout or native callback selector is added. The existing
[fvm_textfield_scroll_info](../include/flashvm/standard_text.h#L68) and path/capture operations remain no-script; they never
synthesize observer events. Layouts are independently owned and destroyed on
failure. Fields without observers can keep a lazy cache; measured candidate caches
are retained after successful geometry publication.

Callbacks run synchronously on the context owner thread under existing native
GC barriers. Collection inside a callback is rejected; roots and execution depth
unwind on success, VM/C++ exceptions, allocation and work errors. Owner-side GC
between calls traces the ordinary listener graph. Binary/channel transfer carries
field state and registrations, not a pending stack-local notification or its
history. Receiver fonts/layouts are independent after sender teardown. Host raw
userdata still needs its ordinary lifetime/capture contract. Thread tests use
separate contexts, not concurrent access to one VM.

## Remaining scope

Text assignment, replaceText, current/future TextFormat changes and font changes
do not yet synthesize geometry events. Full bottomScroll/reference rounding,
onChanged/user input, Selection/focus, HTML/mixed styles and SWF text display-list
integration remain open. Rejected multiline/automatic dimension operations keep
their previous limitations. This is no physical-device or foreign-platform
acceptance claim; audio, timelines, graphics, JIT, concurrency, GC and debugger
roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_textfield_scroll_info` | function | [include/flashvm/standard_text.h:68](../include/flashvm/standard_text.h#L68) |
<!-- source-links:end -->
