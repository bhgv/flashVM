# Scroll observers for text and format edits

The trigger now also includes bottomScroll; descriptor discovery consumes charged work: [visible-line contract](TEXT_VISIBLE_EVENTS.md).


This extends [geometry transactions](TEXT_GEOMETRY_EVENTS.md) to text assignment,
replaceText and current setTextFormat on host-created uniform TextFields. The
[Adobe AS2 reference](https://open-flash.github.io/mirrors/as2-language-reference/TextField.html#onScroller)
describes scroll notifications for programmatic changes and distinguishes them
from user-only onChanged. The bounded trigger and admission policy below are not
an original Flash Player binary oracle. All nine roadmap stages remain open.

## Triggers and version rules

The transaction compares effective scroll, hscroll, maxscroll and maxhscroll.
One changed tuple produces one field-first onScroller event, followed by the
rooted listener snapshot with one actual source-field argument. A text or format
edit with unchanged tuple publishes normally without notification. Changes only
to bottomScroll, intrinsic metrics, autoSize dimensions or origin are not triggers
by themselves. Programmatic edits never synthesize onChanged.

SWF6–8 and unloaded host contexts support these observers. Loaded SWF4/5 keep
their previous silent behavior. Loaded identical counted text preserves the
current format and returns without applying future formatting or notifying.
Unloaded same-text assignment still applies future style and can notify if its
ranges change. setNewTextFormat updates only future formatting; no current
geometry is measured or notified by that operation.

## Content and style publication

[GeometryNotification](../src/standard/text.cpp#L640) now measures proposed counted
content and uniform style independently of the source. Source field geometry is
read after existing argument validation/conversion; explicit conversion effects
remain visible. Potential own/inherited/folded handlers, registration edges and
loaded __resolve use the descriptor admission described in TEXT_GEOMETRY_EVENTS.md;
no getter, resolver or callback runs during notification preparation.

An observed edit must admit both old and proposed layout. Allocation, work,
font/layout or registry preparation failures leave this edit's content, current
style, dimensions and offsets unchanged. Successfully built source caches and
explicit earlier conversion effects may remain. Unobserved fields retain lazy
admission. Registering a potential observer can therefore make deferred layout
errors surface at the edit, including errors in the old source layout.

Text assignment uses counted UTF-8 bytes and the existing future style policy.
replaceText retains existing UTF-16 boundary/count limits and rejects invalid
ranges or split surrogate boundaries. It preserves current uniform style for
nonempty source text; insertion into an empty field uses future style. Proposed
joined bytes and recipient roots are prepared before adopting owned replacement
storage. Allocation/adoption failure does not publish a partial replacement.
Old aliased counted strings remain valid snapshots. No replacement count is
silently clamped to the shorter length of a later call.

Current setTextFormat merges supported uniform properties into a candidate style,
including font selection, size, kerning, leading and supported paragraph/layout
policies. It validates existing font/metric constraints before notification
preparation. Color/underline-only changes are silent when scroll ranges agree.
Mixed style spans and unsupported formats retain their existing errors.

All fallible preparation precedes publication. Content and current style publish
with candidate dimensions, autoSize anchor/origin, owned measured layout and
clamped offsets before dispatch. Handlers therefore observe the new text and
metrics. No public C ABI struct, native callback ID or Field wire bit changes.

## Delivery errors, reentry and ownership

Delivery keeps the established field-first order, snapshot recipient identities
and live handler lookup. First getter/resolver/callback/work failure propagates
and stops the tail, preserving published state and completed effects. There is
no automatic retry or replay. A nested text/style/geometry write remains
authoritative; the outer operation never republishes its old candidate after
callbacks return. Removing a listener does not remove it from the current rooted
snapshot, but later events use the changed registrations.

One execution budget covers preparation and dispatch. Native GC barriers still
reject collection inside callbacks; roots and nesting unwind after success,
allocation/work errors and VM/C++ exceptions. Owner-side GC between operations
traces ordinary graph registrations and can reclaim unreachable cycles. Queries,
path export and binary/channel capture stay no-script and never replay events.
Receivers independently own fonts, text/layouts and listener graphs after sender
teardown. Host userdata keeps its normal lifetime/capture restrictions. Worker
coverage uses independent VMs, not simultaneous access to one context.

## Remaining scope

Full bottomScroll/reference rounding semantics, onChanged/user input, Selection/
focus, HTML/mixed styles, automatic font-source rebinding and SWF text display-list
integration remain open. Automatic reflow caused outside the supported native
setters is not covered. Physical devices and foreign platforms require separate
acceptance. This series does not close audio, video, timeline, graphics, JIT,
concurrency, GC or debugger roadmap stages.
## Validation

TextField content/current-format scroll observers v0.1131–v0.1145: counted text assignment, replaceText and setTextFormat compare proposed content/style geometry before publication. Changed effective offsets or maxima notify field-first and rooted listener snapshots after content/style/layout/autoSize geometry and clamped offsets commit together. Unchanged tuples, future formatting and decoration-only changes stay silent; loaded identical counted text preserves current formatting. replaceText retains existing strict UTF-16 range boundaries and owned/aliased snapshot semantics. Validation passes 185 selected native text/font/graph/tutorial checks, ten focused ASan/UBSan checks plus explicit no-throw callback and no-fault allocation runs; LeakSanitizer is disabled. Ordinary builds sweep allocation/work failures for all three edit operations, distinguish rollback from committed partial delivery, and cover VM/getter/C++ callback errors. Reentrant edits retain nested content/style and rooted removed recipients. Twenty-seven SWF6–8 edit/execution combinations exercise real AVM1 SetMember/CallMethod in interpreter/manual/native JIT with actual native entry counts. Four independent OS workers check 128 text/layout/GC epochs; binary/channel receivers retain listener/font ownership after sender teardown. SWF4/5 remain silent, programmatic onChanged is not synthesized, and autoSize center anchors survive range-neutral edits. SDK-free C89 ABI/admission passes; three font tutorials explicitly skip. A renamed detached component configures with bundled static font dependencies; its copied C89 observer checker passes against the native executable, without a separate detached build. The C89 trace is 1 2 1 2 1 1 1 1 1 1 with final scroll=1/hscroll=0 and maxima=1/40. Full bottomScroll/reference rounding, input/onChanged, HTML/mixed styles, automatic external reflow/font-source rebinding, SWF text display-list integration, devices/foreign platforms and all nine roadmap stages remain open. [Contract](TEXT_EDIT_EVENTS.md).
