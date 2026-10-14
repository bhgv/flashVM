# Standard object graph checkpoint — v0.14

This checkpoint expands the opt-in standard library and its graph channel format.
It does **not** certify completion of all simple types, MovieClip, or SWF 8.
The acceptance gaps below are part of the report, not hidden compatibility promises.
Russian documentation: [STANDARD_OBJECTS.ru.md](STANDARD_OBJECTS.ru.md).

## Object and Array

Object() / new Object() create an object, retain an object argument, or box a
Boolean/Number/String argument. The prototype supplies valueOf, toString,
toLocaleString, hasOwnProperty, isPropertyEnumerable (also propertyIsEnumerable),
isPrototypeOf and addProperty. __proto__ uses the bounded prototype API.
addProperty takes a name, first-class getter and setter (or null setter); its
traced descriptor retains closures and the original receiver across channels.

property.h exposes the AS flags DONT_ENUM=1, DONT_DELETE=2, READ_ONLY=4 and deletion.
ASSetPropFlags changes comma-separated names or all existing own properties.
Data writes and deletion respect flags; Array snapshot mutators retain protected
indexed destinations. Loaded Array protected tails can remain outside length;
readonly length prevents length storage while permitting prior tail deletion
and indexed writes beyond length. Unloaded mode retains protected-tail clamping
and readonly extension guards.

Array stores sparse indices and a uint32 length. Canonical numeric property names
alias indexed entries; leading zero names remain ordinary properties, as does
4294967295. Numeric singleton construction specifies length, whereas an AVM1
InitArray singleton is an element, including negative/fractional numbers.
The methods are push, pop, shift, unshift, reverse, slice, splice, concat, join,
toString, toLocaleString, sort and sortOn. Slice/join/concat use charged work instead of the former range caps and
read inherited indices; join handles recursive arrays. See the detailed
[limitations](ARRAY_CONCAT_SPARSE.md). Sorting supports numeric, descending, uniqueness,
index-preview and ASCII-insensitive options. Numeric strings retain text ordering;
text comparisons use UTF-16 units. sortOn accepts field/option arrays and ignores
an option-array length mismatch. Imported methods can create usable arrays even
without a destination global Array installation. String.split now returns Array.
Loaded sorting follows reference quicksort ordering without equal-item stability;
see [ordering](ARRAY_LEGACY_SORT_ORDER.md) and
[arguments](ARRAY_LEGACY_SORT_ARGUMENTS.md).

## Date and Function

Date stores clipped integral UTC milliseconds, including invalid NaN dates.
UTC conversion uses Gregorian arithmetic across the TimeClip range, without
narrow time_t conversion. Local getters, constructor and setters use serialized
libc calendar adapters in ports/calendar_port.c; clock adapters live separately
under ports/linux, mac, windows and android. Local range and daylight-saving rules
remain those of the host. Tests exercise epoch/leap/rollover/NaN boundaries and
separate UTC, fixed east offset, and winter/summer POSIX TZ processes.

Function is a first-class object for native callbacks and AVM1 DefineFunction
bodies. call/apply use receiver boxing; length and prototype/constructor links
are available. Script functions retain heap lexical activations by identity;
undefined lexical cells shadow globals. arguments contains indexed actual values,
length and callee. Script constructors and AVM1 NewMethod are supported.
Native callbacks retain explicit capture roots; registered callback identities
are required for transfer of host-defined native code. Opaque host userdata and
legacy C-stack closures are not converted into portable addresses.

AVM1 now includes TypeOf, ToNumber, ToString, Add2, Equals2, Less2, StrictEquals,
InitArray and InitObject. Primitive method/property reads box supported receivers.
Date uses its string default hint in Add2/equality; numeric conversion still uses
valueOf first. Metatable properties may hold first-class arithmetic, index,
newindex and finalizer functions; native hooks retain their existing convention.
All nested interpreter frames and native reentry share the outer instruction_limit.

## MovieClip model

movieclip.h creates a host-authored MovieClip with 1-based frames and copied AVM1
frame bodies/labels. Instances have parent/root links, names, sparse depth slots,
ordinary transform fields, editable vector segments and playback state. Methods
cover child creation/removal, depth access/swapping, coordinate conversion,
play/stop/seek/frame movement, solid fill/stroke/lines/quadratic curves, clearing,
bounds/rectangles and bounding-box hit tests. Frame actions and onEnterFrame run
with the clip as receiver. AVM1 Play, Stop, NextFrame, PreviousFrame, GotoFrame,
GotoLabel and GotoFrame2 target that receiver. _root resolves its inherited getter.

Rendering uses statically built AGG or Cairo. NO_GRAPHICS reports UNSUPPORTED for
drawing while retaining bounds and graph transfer. Files/TCP preserve frame
code, drawing commands, playing state, child-parent cycles and class identity;
a fresh receiver can execute frame actions and render the restored model.
This is a host-authored display model, **not playback of a complete SWF display list**.

## Graph and lifetime contract

Queue, file and broker TCP channels use the same value graph snapshot. Version 3
adds watch handlers and their captures; readers accept versions 1, 2 and 3. Version 2
adds property attributes, invocable/constructable entries and cached method
function identities. Compatibility tests include genuine version-1 and version-2 records; older
readers cannot read version 3. Dates, arrays, first-class functions and MovieClip
have explicit serialized state. Graph cycles/aliases and imported prototypes are
preserved. Import does not install global constructors or merge another context's
class identity. Mutex/channel capabilities still refer to their authority rather
than copying the underlying resource.

The host must pin borrowed object pointers while keeping them outside VM roots.
GC traces lexical scopes, native captures, function caches and MovieClip state.
Mutation snapshots allocate before commit. Allocation injection covers atomic
standard installation and all newly installed constructors, followed by GC/retry.
Unknown native callback identities reject restoration and roll back the new heap.

## Acceptance tests and outstanding work

Tests: standard_base.c, array_sort.c, standard_date.c, date_timezone.c,
standard_function.cpp, function_instruction_limit.c, function_metatables.cpp,
avm1_standard_types.cpp, standard_movieclip.c, objects_transfer_process.c,
serialization_validation.c and the expanded error_allocation.cpp.
The object process drivers use independent file writer/readers and TCP processes;
the receiving context does not install the standard library. Raster assertions
check actual filled pixels and untouched exterior pixels after restoration.
Generated results and profiles belong to the separate validation archive.

Remaining gates before the requested completion:

- String's NUL-terminated scalar UTF-8 ABI cannot retain embedded NUL or lone
  UTF-16 surrogates; Unicode case mapping and complete SWF-version semantics
  remain pending. split with an empty separator can therefore reject a cut pair.
- Object.registerClass remains unfinished. Borrowed Array methods, inherited sparse slots and mixed-type sort
  compatibility need further acceptance coverage.
- DefineFunction2/register preloads, complete arguments aliasing, interfaces and
  complete AVM1 control/coercion semantics remain unfinished.
- MovieClip needs SWF display-list/resource integration, attach/duplicate,
  createTextField, dimensions, masks, gradients/bitmaps, shape hit testing,
  drag/input, loaders, filters, blend modes, audio/video and further SWF 8 behavior.
  Nonuniform stroke bounds/rendering are conservative/approximate.
- The actual builds/runs in this checkpoint are Linux. Windows/macOS/Android
  adapters are source implementations, not tested platform certification.

References: [AS2 Array reference](https://open-flash.github.io/mirrors/as2-language-reference/Array.html),
[Adobe SWF specification](https://www.flashrealtime.com/content/dam/Adobe/en/devnet/swf/pdf/swf_file_format_spec_v10.pdf).
