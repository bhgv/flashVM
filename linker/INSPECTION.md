# Validating object and library metadata

`swl_inspect` in [swl.h](include/swl.h) accepts one complete borrowed SWO1 or
SWA1 byte buffer, a non-NULL `SwlModuleVisitor`, opaque caller state, and optional
caller-owned diagnostic storage. It returns 1 after all visits, or 0 on invalid
arguments, malformed input, allocation failure or visitor cancellation.

The same reader used by linking validates checksums, module names, unique names,
record lengths, AVM1 instruction boundaries and branch targets. Every archive
member and the final archive boundary are checked before any visitor runs.
Inspection does not check stack balance or prove that a program terminates.

Each `SwlModuleInfo` gives a borrowed name, code size including ActionEnd, and
requirements mask (bit 0: FlashVM channels). The metadata and name are valid only
during that callback; copy retained values. Members arrive in library order.
Requirements are reported regardless of link permission: inspecting or archiving
an extension object never grants permission to emit it into a movie.

No filesystem, console, shared context or global error state is used. Independent
calls can run on separate threads. The input must remain valid and unchanged for
the call. A visitor may invoke inspection recursively on another buffer. Returning
zero cancels further visits and reports failure; caller side effects from earlier
visits are not rolled back. Prevalidation itself performs no callbacks.

Inspection temporarily owns a normal linker context and copies validated code;
its input/member limits are the same 64 MiB / 1024 limits as linking. It is not a
zero-copy streaming reader. NULL/zero error storage is accepted. As with other SWL
operations, successful calls do not promise to clear old diagnostic text.

The standalone C test [tests/inspect.c](tests/inspect.c) checks a mixed-requirement
library, every truncated prefix, late corruption, cancellation and invalid
arguments. These are format/embedding checks, not foreign-platform execution.

The CLI is a thin adapter: `swl inspect INPUT.swo` or `swl inspect LIBRARY.swa`.
It prints one stable decimal row per member:

```text
module alpha code 1 requirements 0
```

An invalid input produces no rows; errors go to stderr. Success is exit 0, read/
validation/visitor/output failure is 1, wrong argument count is 2. A console write
failure can leave partial text. The command neither emits SWF nor changes input.
Names in SWO1 exclude whitespace, so each row has unambiguous whitespace fields.
The text is a simple CLI report; C applications should call the typed API.
