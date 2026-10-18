# Boolean/prototype checkpoint, development version 0.2

Added boxed Boolean construction, private primitive storage, default prototype
valueOf/toString, class/prototype metadata, rooted constructor capture and direct
numeric unboxing. Primitive Boolean conversion remains distinct from construction.
The new rooted native-method registration API searches prototypes with the
original receiver and copies callback entries before invocation. Data/accessor
shadowing blocks calls; native methods beat same-name AVM1 script functions.
The public ToNumber API now rejects invalid C enum bytes before scalar enum access.
See STANDARD_BOOLEAN.md for supported behavior and compatibility limits.

## Verified builds and tests

Linux x86_64, GCC/G++ 13.3.0, CMake 3.31.10, C89/C++98 with pedantic-errors.
CMake reads VERSION.txt as project version 0.2. Bundled libraries remain static.

| Profile | Passed tests/examples |
| --- | ---: |
| AGG | 125 |
| Cairo | 125 |
| NO_FONTS | 121 |
| NO_GRAPHICS and NO_IMAGE_CODECS | 119 |
| ASan/UBSan (AGG) | 125 |

scripts/check.py --sanitize rebuilt all selected core and vendor sources, then
compiled, linked and executed every test/example. Recovered diagnostics are
also rejected. LeakSanitizer is disabled because /proc is unavailable; the
historical custom allocator exception test uses UBSan only. The final source
timestamps precede their instrumented object timestamps, including the parent
method override added to the worker test. GCC also reports the existing
AVM1 scoped-root address warning; the root guard restores the previous frame,
and explicit/automatic GC tests pass. No warning-free build claim is made.

tests/standard_boolean.c covers omitted/undefined/null/boolean/numeric/string/object
inputs, NaN/infinity/signed zero, wrapper truthiness versus primitive extraction,
numeric conversion, prototype metadata and invalid receivers, extra arguments,
constructor capture after global removal, explicit GC and automatic AVM1 GC
pressure, channel rejection, foreign objects, invalid C enums and atomic name
conflicts. tests/native_prototype_methods.c covers receiver/argument binding,
native-versus-script precedence, own overrides, data/accessor shadows, capture
retention/replacement/finalization, self-replacement, legacy fallback and the
1024/1025-object chain boundary. tests/actionscript_extensions.cpp runs eight
workers using their own Boolean namespaces after overriding the parent method;
each constructs, collects with a live local root and returns a primitive string.

The C89 Boolean embedding example actually executes supported operations.
example.as is illustrative source; direct AVM1 fixtures supply execution evidence.
Reports/logs are in validation/boolean-*.json and *.log. Two existing binaries
initially lacked execute permission; restored bits and final full reruns passed.
The unchanged vendor manifest verifies 2094 files.

## Version/archive and mirror policy

The requested policy is now recorded in AGENTS.md: increment VERSION.txt before
each new commit, start the English subject with that version, and end archive
names with -v<version>.zip. ABI version numbers remain independent.

On the explicit command, the mirror was fast-forwarded from 0a9c53d to 6831bf4
with a 64 MiB HTTP buffer and no force push. Fetch plus ancestor/hash comparison
confirmed server master equals the internal commit. Subsequent version 0.2
development is NOT pushed. The internal repository/archive remains authoritative.

## Outstanding scope

This checkpoint does not certify full Boolean or Flash compatibility. First-class
Function values, primitive boxing, Object inheritance/attributes, generic
ToPrimitive, complete operators/version gating and remaining standard/system
objects are unfinished. Actual Windows/macOS/Android execution is not verified.
