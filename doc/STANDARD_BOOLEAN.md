# Boolean wrappers and native prototype methods

`fvm_standard_install` now installs global Boolean class metadata and a dedicated
Boolean constructor alongside the existing numeric functions. `Boolean(value)`
through fvm_call/AVM1 CallFunction still returns a primitive; `new Boolean(value)`
through fvm_construct/AVM1 NewObject returns a GC-owned object. Fixed-arity
conversion and methods ignore extra valid arguments. Installation remains opt-in,
rejects conflicting names, and publishes globals/functions/constructors together.
Repeated installation verifies the class, prototype methods and constructor
capture, reporting INVALID_ARGUMENT if a checked binding has been replaced.

The method names, constructor default and primitive/string result types follow
the [Adobe AS2 Boolean reference](https://open-flash.github.io/mirrors/as2-language-reference/Boolean.html).
This reference does not establish full Flash version differences or runtime parity.

| Input | Stored Boolean primitive |
| --- | --- |
| No argument, undefined, null | false |
| false, +0, -0, NaN, empty string | false |
| true, nonzero numbers, infinities, nonempty strings | true |
| Any same-context object, including a false Boolean wrapper | true |

`valueOf()` returns the stored primitive; `toString()` returns "true" or "false".
The default Boolean prototype itself stores false. A generic object merely
inheriting this prototype cannot impersonate a Boolean wrapper: methods reject
that receiver. Ordinary user properties do not alter the private primitive.
The current Number conversion unboxes Boolean directly to 0/1; it does not
execute user overrides of valueOf/toString. General object ToPrimitive remains
unsupported. Boolean(wrapperFalse) is true because the input is an object.

## Native method lookup and extension API

`fvm_object_define_method_rooted(context, object, name, callback, userdata, root)`
defines/replaces an own native callback. Root must be null or belong to the same
context; arbitrary userdata memory is not scanned. Existing own data/accessor
bindings reject registration. Replacing a callback releases its previous capture
root. fvm_object_has_own includes own native methods; inherited ones are not own.

Both fvm_object_call and AVM1 CallMethod search at most 1024 objects in the
prototype chain and bind the original receiver, not the object storing the
method. Own callbacks override inherited ones. Data/accessor bindings encountered
first block native invocation, returning UNSUPPORTED: callable property values
and accessor-returned functions are not yet dispatched. A data assignment can
therefore shadow a previously registered callback without removing its capture.
The old capture remains until callback replacement or collection of the owner.

The selected callback entry is copied before invocation, so it can safely replace
itself. Native guards prevent GC reentry; inherited callback captures remain
reachable through the receiver/prototype graph. Only absent methods permit the
existing named-function bridge fallback. In AVM1, a native method anywhere in
the supported prototype chain takes precedence over a same-name script function.
Missing methods beyond the chain budget return ERROR rather than falling back.
This is a bounded native bridge, not a complete first-class Function implementation.
Method values are not exposed as function-valued properties by fvm_object_get.

## GC, workers and examples

Class/prototype constructor cycles are traced. The constructor registry captures
the class, so deleting the global Boolean binding does not lose the constructor's
resources. As with all returned object pointers, hold a pin/root while native code
keeps a wrapper. Since development 0.3, context-aware channels serialize boxed
Boolean state and built-in prototype methods, including file channels.
See FILE_CHANNELS.md; arbitrary native callbacks still require registered identities.

ActionScript workers recreate the opted-in standard namespace in their own
contexts; parent prototype changes are not copied. Tests run eight workers with
local Boolean construction, GC and primitive string results while the parent's
toString method is overridden. C89 `examples/embedding/standard_boolean/main.c`
executes construction, primitive extraction, string conversion and the distinction
between object truthiness and numeric unboxing. `example.as` is illustrative;
the tests separately execute actual AVM1 bytes, without claiming an external
ActionScript compiler corpus.

## Remaining compatibility work

Object/Function inheritance and attributes, first-class methods, automatic boxing
of primitive receivers, generic ToPrimitive, complete operator coercion,
SWF-version gating and boxed Number/String remain unfinished. No full Boolean
or SWF 8 compatibility is claimed. No Windows/macOS/Android execution is verified.
See STANDARD_BOOLEAN_CHECKPOINT.md for build/test evidence and mirror status.
