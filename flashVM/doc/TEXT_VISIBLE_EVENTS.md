# Visible lines and bounded observer lookup

This extends [text/edit transactions](TEXT_EDIT_EVENTS.md) to the bottom visible
line and charges descriptor discovery before geometry publication. The
[Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#bottomScroll)
describes the read-only bottomScroll visual-line index and scroll-property
notifications. The analytic geometry and transaction rules below are bounded
implementation policy, not a Flash Player binary oracle. All nine stages remain open.

## Five-property trigger

The observed native edit transaction now compares scroll, hscroll, maxscroll,
maxhscroll and bottomScroll. One changed tuple emits one synchronous field-first
onScroller event, followed by the rooted registration snapshot. A change only to
bottomScroll now counts: for example, text changes from three fitting lines to
one while offsets and maxima remain scroll=1/hscroll=0/maxscroll=1/maxhscroll=0.

This applies to supported dimension/layout-policy writes, counted text assignment,
replaceText and current setTextFormat. It does not make bottomScroll writable.
Unchanged five-property tuples, color/decoration-only edits and future formatting
remain silent. A size, layout metric or autoSize origin change alone is not an
additional trigger. Loaded identical counted text retains its existing early
return/current-format behavior. SWF4/5 stay silent; SWF6–8 and unloaded host
contexts support the event. Programmatic edits do not synthesize onChanged.

## Descriptor-only work accounting

[fvm_textfield_observer_present](../src/standard/text_listeners.cpp#L73) is a private C++98 owner-thread helper, not a new
public C ABI entry. It validates receiver/lifecycle/version and leaves its Boolean
output false on status failure. Before admitting measurement it checks the private
registration edge or potential own/inherited onScroller descriptors and loaded
__resolve descriptors, without invoking getters, resolvers or callbacks.

Exact spelling is selected first. Loaded SWF6 folded fallback uses the same UTF-16
name equivalence and first matching stored spelling as ordinary property lookup;
host/SWF7–8 are exact. Attribute visibility is resolved with the same charged
selection. Exact table lookups, prototype visits and each entry visited during
folded fallback consume shared native execution work. Traversal checks owning
contexts and the existing prototype-depth bound. Cycles, foreign graph segments,
work exhaustion and periodic worker cancellation do not publish an edit.

Discovery shares the existing [FVM_TextFieldScrollPlan](../src/standard/text_listeners.hpp#L28) execution budget through
candidate preparation and dispatch; it does not reset the budget before callbacks.
Fixed lookup keys may allocate, with the ordinary public-call allocation status
mapping. Bounds count visited tables/entries, not each byte of a stored name or
every font-engine operation. Potential noncallable descriptors and an empty
previously created registry still admit measurement. Missing potential observers
retain lazy font/layout admission, but descriptor discovery itself consumes work.
These are conservative potential-observer checks, not a claim that every found
descriptor will later resolve to a callable handler.

## Publication and delivery

Old and candidate layouts must satisfy the existing observed-edit admission
contract. Preparation errors preserve the edit's old text/current style/geometry/
offsets; completed conversion effects and successfully built source caches may
remain. After successful preparation the transaction publishes content/style,
layout, autoSize geometry and clamped offsets before any handler lookup runs.
Handlers read the committed bottomScroll and other properties. Query/path export
and graph capture remain no-script and never synthesize/replay events.

Recipient identities/order are snapshotted and rooted; each handler is resolved
live. A removed recipient can still receive the current event. Nested edits keep
their final text, style and geometry. First getter/resolver/callback/work error
stops delivery and propagates without rolling back published state or completed
effects. There is no automatic retry/replay. Existing callback GC barriers remain;
owner-side collection between operations traces the ordinary listener graph.

No public struct, Field wire bit, callback selector or graph format is added.
Binary/channel receivers independently own fonts/text/listeners after sender
destruction, with no pending stack-local notification transferred. Raw host
userdata retains its ordinary lifetime/capture restrictions. Worker coverage uses
independent VMs, not simultaneous access to a shared context.

## Remaining scope

Complete original-player rounding/edit behavior, onChanged/user input, HTML/mixed
styles, automatic external reflow/font-source rebinding, Selection/focus and SWF
text display-list integration remain open. Descriptor-name byte processing and
font work are not newly metered by this series. Devices and foreign platforms
require separate acceptance. Audio/video/timeline/graphics/JIT/concurrency/GC/
debugger roadmap stages remain open.

## Validation

TextField visible-line observers and metered discovery v0.1146–v0.1159: supported native geometry/text/current-format edits now compare all five scroll properties, including bottomScroll. A changed bottom visible line notifies even when offsets and maxima agree. Descriptor-only admission preserves exact-first SWF6 folding/visibility and inherited/resolver discovery, charging exact lookups, prototype visits and every visited fallback entry through the shared preparation/delivery work budget. Validation passes 196 selected native text/font/graph/tutorial checks, ten focused ASan/UBSan checks plus explicit no-throw callback and no-fault allocation runs; LeakSanitizer is disabled. Allocation sweeps additionally cover all three edit kinds with and without the registry fast path. Ordinary tests distinguish rollback from committed partial delivery and cover VM/getter/C++ errors. Twenty-four version/name/descriptor combinations invoke no probe-time getter; large SWF6 tables exhaust work before publication and cover committed work failures. Cycles/foreign graphs and output/depth cleanup are checked. Twenty-seven real SWF6–8 edit/execution combinations exercise interpreter/manual/native JIT with native entry counts. Eighteen host/SWF4–8 edit combinations preserve old-version gates and no programmatic onChanged. Nested edits retain final content/style and rooted removed recipients. Four independent OS workers check 128 bottom-only text/GC epochs; binary/channel receivers keep fonts/listeners after sender destruction. SDK-free C89 ABI/admission passes, three font tutorials explicitly skip. A renamed detached component configures with bundled static font dependencies; its copied C89 checker passes against the native executable, without a separate detached build. C89 callback trace is 1 2 1 2 1 1 1 1 1 1 1; bottom-line trace is 2 2 2 2 3 3 3 1 1 2 1, final offsets=1/0, maxima=2/40, bottomScroll=1. Full original-player rounding/edit semantics, input/onChanged, HTML/mixed styles, external reflow/font-source rebinding, Selection/focus, SWF display-list text, devices/foreign platforms and all nine roadmap stages remain open. Name-byte processing and font work are not newly metered. [Contract](TEXT_VISIBLE_EVENTS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_textfield_observer_present` | function | [src/standard/text_listeners.cpp:73](../src/standard/text_listeners.cpp#L73) |
| `FVM_TextFieldScrollPlan` | class | [src/standard/text_listeners.hpp:28](../src/standard/text_listeners.hpp#L28) |
<!-- source-links:end -->
