# Value types and object lifetimes in the FlashVM–DisVM bridge

[Русский](STYX_RPC_VALUE_TYPES.ru.md) · [Bridge setup](STYX_RPC.md) · [Runnable paired example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md) · [Live VFS variables](STYX_RPC_LIVE_VARIABLES.md)

FlashVM and DisVM keep their own heaps. A typed call checks the compiled Limbo signature and positional schema, then uses the same UniBus S-records as the C SDK. The selected RPC profile determines whether the result is a copy, an update to an existing AS object, or a handle for later operations.

## Choose how to transfer the data

| Need | Interface | Lifetime and behavior |
| --- | --- | --- |
| Pass strings, scalars, arrays, lists or tuples to a function | [Managed import](STYX_RPC_IMPORT_MANAGED.md), [managed export](STYX_RPC_EXPORT_MANAGED.md) or [checked `file.load()`](STYX_RPC_LIVE_DIS.md) | Each side owns the decoded values. Assigning to an element on the receiving side leaves the sender's array unchanged. Return a value to transfer the edit. |
| Allow a function to modify a flat `ref ADT` argument | [Ordinary ref arguments](STYX_RPC_REFS.md) | The AS Array contains the ADT fields in order. A valid reply copies changed fields back into the original Array. |
| Send nested refs, arrays of refs, returned refs or hybrid tables | [Graph import/export](STYX_RPC_GRAPH.md), [hybrid tables](STYX_RPC_HYBRID.md) | One call preserves ref identity and can apply sparse changes to existing objects. A managed hybrid table carries string keys, signed 64-bit numeric keys and signed 64-bit values. |
| Preserve holes through a compiled typed call | [`packSparse`](STYX_RPC_PACK_SPARSE.md), [`unpackSparse`](STYX_RPC_UNPACK_SPARSE.md) | Logical length and sorted index/value pairs. Limbo uses `(int, array of (int, T))`. |
| Update a ref or table and acquire a channel in the same call | [Scoped channel profile](STYX_RPC_CHANNELS.md#combine-a-graph-with-a-channel) | The validated reply binds updates to the original objects and issues a separate channel lease. |
| Send a sparse Array or portable object graph | [Named `port.send(value)`](STYX_RPC_NAMED_SEND.md), [Limbo `Styxnamed`](../../disVM/runtime/STYX-NAMED-VALUES.md) | Snapshot with own data, holes, shared identities and cycles for supported FVMG values. The receiver owns its own graph. |
| Read or replace a value shared by several holders | [Named `port.send(ref slot)`](STYX_RPC_NAMED_SEND.md), [C/Limbo example](../../lib/styxrpc/examples/shared-cell/README.md) | Live lease on a writable slot or own property. A read of a graph yields a snapshot. Reassign the edited snapshot to publish it, or share an individual writable slot such as `ref rows[index]`. |
| Read and replace native data through VFS | [Live variable binding](STYX_RPC_LIVE_VARIABLES.md) | Int32, an Int32 array, `big`, a `big` vector, numeric hybrid keys, a string and named record fields. Each request targets the held node on its issuing session. |
| Access an exact exported native integer | [Mounted Int32 binding](STYX_RPC_INT_VARIABLE_BIND.md), [forwardable module Int32 ref](../../disVM/runtime/STYX-MODULE-REFS.md) | Live read/write operations on the issuing session. The exact module reference verifies export identity and Qid. |
| Call a method later or exchange values through a queue | [Loaded module proxy](STYX_RPC_LIVE_DIS.md), [typed channels](STYX_RPC_CHANNELS.md) | The module proxy holds the loaded image and its global state. A channel lease keeps a route live until release. Its ordinary payload values use the typed copy rules. |

A function can return a temporary graph and reuse its objects within that caller. A separate call has its own identity table. Use a shared cell, a live VFS binding or a channel when effects must be visible after the call has completed.

## Read a typed schema

| Limbo type | Schema entry | AS input and decoded result |
| --- | --- | --- |
| `int`, `byte` | `i`, `y` | Integral `Number` in −2147483648…2147483647 or 0…255, respectively. |
| `big` | `b` | Integral `Number` in the safe interval, or counted decimal `String` over the full signed 64-bit range. The decoded type follows the rules below. |
| `real` | `f` | Binary64 `Number`. |
| `string` | `s` | Counted Unicode `String`, including embedded zero, or `null` for nil. An ordinary numeric-looking string retains its text. |
| `array of T` | `a[T]` | Dense indexed AS values of the element shape, or `null` for nil. Each nested array has its own length. |
| `list of T` | `l[T]` | Dense Array of copied elements or `null` for nil. An empty Array also encodes nil. |
| Tuple | `(T1T2…)` | An Array in declaration order with exactly the declared arity. |
| `ref ADT` | `r[T1T2…]` | Positional Array or `null` for nil. Its permitted position and copy-back rules depend on the ref profile. |
| Managed hybrid table | `h` | Table with separate numeric and string key spaces, carried in the graph profile. |
| `chan of T` | `c[T]` | Live channel object or `null` for nil, in the scoped channel profile. Its element uses shape `T`. |
| No result | Empty result list `()` | `undefined`. |

A complete schema is an argument list, `=>`, and a result list containing either one value or nothing. `(a[s]a[b]) => ((a[s]a[b]))` takes two arrays and returns one tuple of two arrays. The outer result parentheses enclose the single tuple type.

The FlashVM typed conversion caps each array at 2,048 entries and uses six levels per schema item. The managed bridge caps encoded frames at 64 KiB. Check the signatures at both ends, including the differences between a nil array, a zero-length array and a missing element. A hole in a typed array fails conversion. Typed collection reads can run getters on the VM owner. The named graph API applies its own snapshot policy and works with sparse data.

To preserve holes, call `vfs.packSparse(values)` before the call. The schema for element type `T` is the tuple `(ia[(iT)])`. For example, use `(ia[(is)])` for strings. After the call `vfs.unpackSparse(reply)` restores the logical length and the specified own indices into a new Array. An ordinary `array of T` uses the dense mapping. [Example with a moved index](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md#send-a-list-or-sparse-array-as-a-copy).

## Pass a signed 64-bit integer

For `b`, pass a finite integral `Number` within ±9,007,199,254,740,991, or an ASCII decimal string in the range −9,223,372,036,854,775,808…9,223,372,036,854,775,807. The string grammar is `[+-]?[0-9]+`. A sign and leading zeros are permitted on input. Whitespace, embedded zero bytes, exponent notation, a decimal point and overflow fail conversion before submitting the call.

The result is a `Number` inside the safe interval. Outside it the result is a canonical decimal `String` with no leading zeros or plus sign. For example, `"-00042"` comes back as the number `-42` through an echo. `"+09223372036854775807"` comes back as `"9223372036854775807"`. Use a quoted decimal when supplying a large literal. Arithmetic on those values belongs in DisVM `big` or a C signed 64-bit calculation.

The rule applies to typed arguments, results, array elements, tuple fields, ref copy-back and channel payloads. It also applies to FlashVM's [hybrid table binding](STYX_RPC_HYBRID_BIND.md). On the wire each of these integers is a signed `big`, including when AS supplies a decimal string. [C encoder](../../lib/styxrpc/reference/value_integer.md) · [Managed codec](../src/styxrpc/managed_values.c)

A named message has no compiled `b` position. Sending the same quoted literal with `vfs.chan` or storing it in a named cell keeps it as text.

## Limits

- Ordinary managed copy-back handles up to eight flat ref arguments. The graph profile allows up to 256 identities per call, eight direct fields per ref Array and eight flattened fields per sparse update. Graph copy-back checks the entire response before modifying the caller's slots.
- A `list of T` copies its elements. Scalars, strings, tuples, dense arrays and nested lists can appear as elements. A list or array can contain up to 2,048 items. On the wire a nonempty list uses the sequence record tag 8 and nil uses tag 9. Functions, prototypes and native file handles are outside the typed schema.
- A scoped schema admits `r[…]`, `h` and `c[…]` in separate positions of the same call. Copied ref fields cannot contain channels. Channel messages use value types and supported nested channels. A live VFS record exposes fields explicitly supplied by its owner.
- Live Dis loading is session opt-in. Discovery admits up to eight callable exports per loaded image, with up to 32 concurrent loading or retained generations per session. Check the profile, ABI and compiled metadata constraints in the [DisVM loader](../../disVM/runtime/VFS-SCALAR-LOADED-EXPORTS.md) and [player reference](STYX_RPC_LIVE_DIS.md).

For a synchronous call that fails after submission, consult the call's status and the application's own state before replaying it. Release a module proxy with [`os.unload()`](STYX_RPC_UNLOAD.md), close [typed channels](STYX_RPC_CHANNELS.md) and retire [named port and cell bindings](STYX_RPC_NAMED_CHANNEL.md) when they go out of use.

[Composite result and owner endpoint](STYX_RPC_RESULT_CHANNELS.md).
