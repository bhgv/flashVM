# File channels and value metatables — development 0.4, ABI 2

The descriptor `<"file", path, mode>` corresponds to `channel.create("file", path[, mode])` in the opt-in ActionScript bridge. The C89 entry point is `fvm_channel_open_file(path, mode, &channel)`. `path` is a nonempty UTF-8 path. `mode` is exactly `r`, `w`, or `rw`; omission/NULL means `rw`. Empty strings, other modes and invalid descriptor argument types are rejected.

| Mode | Read | Write | Opening behavior |
| --- | --- | --- | --- |
| `r` | Yes | Error | Existing file, unchanged |
| `w` | Error | Yes | Create or truncate |
| `rw` | Yes | Yes | Preserve or create; append |

Each handle has independent read and write positions. Reading starts at byte zero. A successfully sent message is flushed before send returns. Operations on one retained/shared handle are protected by its channel mutex. Independent handles/processes are not protected by that mutex: use a single shared writer handle. This adapter uses stdio/`long` positions; it does not promise files beyond `LONG_MAX` on platforms with 32-bit `long`. Flush is not a crash-atomic transaction or an fsync durability guarantee.

`send`/`receive` and `trySend`/`tryReceive` use typed serialized records. At EOF, nonblocking receive returns `FVM_NOT_FOUND` (`tryReceive()` returns undefined); blocking native receive returns `FVM_ERROR`; the AS wrapper throws `resource.io`
(`FVM_EXCEPTION`). It never waits indefinitely for another process to append. Read/write permission errors and operations after close throw resource exceptions
from AS methods; low-level native calls retain error statuses. See [exceptions](EXCEPTIONS.md). `available` checks for remaining bytes; it does not validate the next record. Queue capacity and pending-message count are unsupported for file channels.

`fvm_channel_file_read` and `fvm_channel_file_write` provide raw byte access to ordinary files, including embedded NUL bytes. These operations use the same read/write positions. Do not interleave raw data and typed records unless the raw data forms complete records. The ActionScript bridge currently exposes typed messages only.

## Persistent graph format

`fvm_value_serialize` returns an owned buffer; release it exactly once with `fvm_serialized_free`. `fvm_value_deserialize` restores into a destination context. Both memory graph channels and file message channels use this format. A queued snapshot retains no source VM pointers and remains usable after source context destruction.

Format 1 has a 16-byte envelope: `FVMG`, three zero bytes, version byte 1, big-endian 32-bit body length, and big-endian CRC32 of the body. Numbers use IEEE-754 binary64 in big-endian order. Object references are numbered graph IDs, not addresses. The format is FlashVM-specific, not SWF, AMF or Lua serialization. Unsupported floating-point representations are rejected. CRC detects accidental corruption; it is not authentication.

Snapshots preserve named/indexed properties, prototype and metatable edges, cycles, shared object identities, native methods/accessors and registered metamethod captures. Primitive value metadata is also transferred. A receiver obtains an independent graph; sending does not move the original objects. Prototype cycles are invalid. Capture and decode are bounded by 4096 objects, 65536 entries and 64 MiB per record. Serialized indices are unsigned 32-bit. Aggregate text/string/font data is checked before copying during capture. Malformed lengths, tags, references, duplicate keys, callback descriptors, CRCs and truncated records are rejected. Failed object restore rolls back objects, strings, allocation counters and fonts without running user finalizers.

| Native object | Current codec |
| --- | --- |
| Ordinary objects/tables | Properties, indexed values, prototypes, metatables |
| Boxed Boolean and Boolean class metadata | Private Boolean value, prototype methods and class graph |
| Math | Properties/methods and source context PRNG state; restore updates destination context PRNG |
| TextFormat and text class metadata | Properties/methods; standalone TextFormat measurement still needs its referenced/default font registered in destination |
| Host-created TextField | Text, current/future style, dimensions, wrapping/multiline flags, methods/accessors, used font bytes and face indices; rebuild layout caches |
| Live workers/channels, arbitrary userdata, stack callables | Rejected until a resource/callable codec exists |
| Extension namespace objects (gc/thread/channel/metatable) | No namespace codec yet; rejected |
| MovieClip and other unimplemented Flash classes | No runtime implementation/codec yet |

Install the destination text bridge with `fvm_standard_text_install` before restoring TextField. Embedded fonts are registered transactionally; an existing name must have exactly the same bytes and face index. Conflicts reject restoration. Current/future font faces are embedded separately, so records are bounded but not deduplicated. Faces use signed 32-bit indices in this format. All normal restrictions of the implemented TextField subset still apply.

Application native callbacks require the same stable name registered in sender and receiver with `fvm_serialization_register_callback`. Native code addresses are never stored. Userdata must be NULL or exactly the callback's traced root; the root is cloned and rebound. Arbitrary userdata memory and collector-entry callbacks are rejected. Built-in callback identities and TextField selector arguments use reserved internal IDs. Registration cannot override built-in code. A missing application callback rejects restore. Native callback registration does not transfer named AVM1 function definitions or executable bytecode.

A primitive-only C receive returns `FVM_UNSUPPORTED` without consuming an object/metadata graph, allowing a context-aware receive afterward. Invalid/truncated file framing leaves the read cursor unchanged. Context-aware receive consumes a fully framed record even if its payload/codec fails to restore, as memory queues consume rejected packets. File access failure during a partial write disables subsequent operations on that handle; the tail may require external recovery.

## Metadata on every existing value type

`fvm_value_set_metatable` / `fvm_value_get_metatable` work with undefined, null, Boolean, number, string and object values. Primitive metadata belongs to the value and follows assignment/copy; replacing a primitive with `fvm_value_number`, `fvm_value_string`, etc. clears it. Object metadata belongs to shared object identity, including supported built-in objects. Metatables must belong to the same context. Clearing attachment uses NULL.

ActionScript uses `value = metatable.attach(value, table)` and `metatable.get(value)`; passing null/undefined as the second argument removes attachment. This namespace is installed by `fvm_extensions_install`. Metadata is traced through globals, table entries, indexed entries, AVM1 stacks/registers/locals, native capture roots and explicit C root scopes. A C local is not automatically a GC root: use the existing root-scope API or a global/pinned object.

`fvm_value_meta_invoke` dispatches a registered native hook with `[ownerValue, definingMetatable, operationArguments...]`. The callback receiver is NULL for primitives and the owner object for objects. Missing hooks return `FVM_NOT_FOUND`. Hook lookup follows bounded metatable chains. Callback output is validated for destination-context ownership.

Registered names: `__index`, `__newindex`, `__iterate`, `__gc`, `__call`, `__add`, `__sub`, `__mul`, `__div`, `__mod`, `__pow`, `__unm`, `__eq`, `__lt`, `__le`, `__concat`, `__tostring`. Generic value property access and AVM1 GetMember/SetMember call primitive `__index`/`__newindex`. AVM1 arithmetic Add/Subtract/Multiply/Divide and Equals/Less consult hooks, left operand first, then right; operation arguments remain in original left/right order. Hooks for other operations can be invoked explicitly but are not yet wired into all opcodes/coercions. Existing object indexing, iteration, invocation and finalization APIs remain available. `__gc` finalizes objects with stable identity, not individual scalar copies.

ABI 2 adds `FVM_Value.metatable`. Rebuild all native clients and always initialize values with the provided helpers before setting fields. Plain primitive channel/worker C APIs cannot retain context pointers and reject attached metadata; use context-aware graph channels for transfer. Native worker argument/result graph transfer is still pending.

## Examples and evidence

Executable C89 example: `examples/embedding/file_channels/main.c`. ActionScript bridge usage: `examples/actionscript/file_channels.as`. The `.as` example is documentation; tests execute AVM1 bytecode and C bridge calls. Angle-bracket descriptors, arrows and other custom source syntax still need an extension-aware compiler.

Regression coverage: `tests/file_channels.c`, `tests/value_metatables.c`, `tests/serialization_validation.c`, `tests/serialization_textfield.c`, plus the existing graph/GC/native-prototype/worker suites. Build profiles and actual results are recorded in `FILE_CHANNELS_CHECKPOINT.md`. This release does not certify every Flash class, arbitrary closures or cross-platform runtime execution.
