# Binary S-record profile 1

[Русский](SRECORD.ru.md) · [Public C89 API](../include/flashvm/srecord.h) · [Roadmap](ROADMAP_STATUS.md).

This implements the user-proposed binary framing, not textual S-expressions.
The first profile fixes otherwise unspecified choices explicitly. It is not an
existing 9P/Styx wire dialect and is not automatically enabled on VM channels.
No VM pointers, native structs or machine byte order are serialized.

## Integers

Octets are most-significant-group first. Bit 7 marks the **last** octet. Bit 6
of the first octet is the sign; its other six bits start the magnitude. Every
later octet contributes seven bits. This is signed magnitude, not zigzag or
LEB128. Canonical encoding is mandatory; redundant groups and negative zero
are invalid. This implementation accepts magnitudes through 2^64-1 using two
32-bit C89 limbs, including on machines without a 64-bit unsigned long.

| Value | Hex octets |
| --- | --- |
| 0 | `80` |
| 63 | `bf` |
| -63 | `ff` |
| 64 | `00 c0` |
| -64 | `40 c0` |
| 8191 | `3f ff` |
| 8192 | `00 40 80` |

No record-level byte swapping applies to opaque payloads. Schemas for float32/
float64, samples, timestamps and image formats must specify their own exact
big-endian representation. Codec read/write alone does not reinterpret payloads.

## Framing and checksum

```text
checksum : unsigned canonical integer
length   : unsigned canonical integer
body     : unsigned canonical tag followed by opaque payload
```

Length counts the encoded tag plus its payload; it excludes checksum and length
fields. Prefer tags 1..63 for common scalars and larger tags for bulk/container schemas.
Tags are nonnegative 32-bit values. Zero is reserved for direction-local
binding metadata. Record bodies are nonempty and limited to 16 MiB; a reader
also supplies a lower per-call quota. Empty payloads and embedded NULs are valid.
Payloads can contain nested complete S-records, chunks or schema-defined metadata.
The schema determines which; framing does not recursively execute/parse content.

Checksum is calculated over the exact encoded **length + tag + payload** bytes.
Provisional profile 1 uses CRC-32/ISO-HDLC: reflected polynomial edb88320,
initial ffffffff, final xor ffffffff. Retain the low min(32, 6+7*(L-1)) bits,
where L is the encoded length-field width. Encode the resulting nonnegative
integer canonically. Short bodies therefore have a smaller checksum-width bound;
an unusually small CRC value may encode even shorter. Checksum width is not a
strict monotonic function of body length. With the current 16 MiB cap at most
27 CRC bits are retained; one-octet lengths retain six. This detects accidental
corruption with limited strength and is not authentication or integrity against
an active peer. Security belongs to the authenticated transport/capability layer.
Changing this rule requires an explicitly different out-of-band profile, not
silently reinterpreting existing bytes. No prefix magic/version was added to the
user's specified record layout.

The reader validates canonical framing and quotas before considering body data.
It verifies checksum before interpreting the tag or exposing a view. MORE means
an incomplete prefix, not success. Corrupt framing is not automatically scanned
past or resynchronized; transports must enforce buffer/time quotas and choose
whether to close/reset the session. A checksum does not make an untrusted length
safe to allocate without bounds.

## C API ownership and errors

| API | Contract |
| --- | --- |
| [`fvm_sinteger_write`](../include/flashvm/srecord.h#L56) | Borrows integer and caller output; validates 32-bit limbs/sign and full capacity before writing. OK publishes used; errors preserve bytes/used. |
| [`fvm_sinteger_read`](../include/flashvm/srecord.h#L58) | Borrows prefix; requires output value/used, publishes only on OK. MORE retains neither input nor partial result; caller retains and retries prefix. |
| [`fvm_srecord_write`](../include/flashvm/srecord.h#L66) | Borrows tag/payload, writes caller buffer only after complete validation. Payload and outputs must not overlap. No allocation, VM or transport. |
| [`fvm_srecord_read`](../include/flashvm/srecord.h#L72) | Borrows input and returns one verified borrowed view/consumed extent. limit counts body bytes. Outputs unchanged on MORE/error; trailing records remain for another call. |

All calls are reentrant with independent storage, contain no global mutable
state and do not enter VM, GC, SDL or devices. Caller serializes shared buffers.
Pointers must address valid accessible storage and output objects must not alias
each other/input. No pointer accessibility/provenance probing is performed.
[FVM_S_OK](../include/flashvm/srecord.h#L30), MORE, INVALID, LIMIT and CHECKSUM distinguish complete/incomplete,
malformed, bounded and damaged data. No exception crosses C. Native views are
never sent by copying sizeof(struct) bytes.

## Build and test

From workspace root, with a native C89/C++98 compiler, CMake and Python 3:

```sh
cmake -S flashVM -B build-srecord -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-srecord --target flashvm_srecord flashvm_srecord_oracle --parallel
ctest --test-dir build-srecord -R '^flashvm_srecord_' --output-on-failure
```

The standalone static target `flashvm_srecord` contains only protocol C sources.
The same sources are included in the normal VM static SDK, so applications use
either link route; no SDL/network/VM startup is required for codec operations.
The test-only shared oracle exposes the C codec to independent Python big-integer
and zlib vectors. Cross builds supply a target toolchain file; tests run on the
target. The codec uses C89 limbs and explicit bytes, not host endian assumptions.
Foreign native execution and 9P/Inferno filesystem integration remain future work.
## Direction-local tag-zero bindings (v0.723)

[sbindings.h](../include/flashvm/sbindings.h) adds caller-owned bounded registries.
Use separate registries for every session direction: A's outgoing declarations
populate B's incoming table; B's outgoing declarations populate A's incoming table.
Tag 3 can therefore mean `ieee754.f64.be` A→B and `boolean` B→A. There is no global
numeric type map. Tag zero cannot be rebound. Unbound nonzero tags remain unknown;
the raw codec may expose their bytes, but dispatch must not guess their meaning.

Tag-zero payload schema 1 is binary:

```text
unsigned payload_schema_version = 1
unsigned binding_count
repeat binding_count:
    unsigned nonzero_tag
    unsigned nonzero_type_schema_version
    unsigned identity_byte_count
    identity_bytes
```

All integers use the same canonical encoding. Identity is 1..63 printable ASCII
bytes excluding spaces; it names a schema, not a codec or permission. Content
strings/UTF-16/UTF-8/binary data are unaffected by this metadata-name restriction.
At most 256 associations exist in this first bounded registry. Payload count zero
is valid. Nested application records require their own declared sequence schema.

Bindings are immutable during a direction/session. Exact repeats are idempotent;
a duplicate tag within one declaration is invalid even if identical. A conflicting
binding, malformed/truncated/checksum-damaged payload, extra payload bytes or
insufficient slots leaves the entire table unchanged. Reset is explicit table
reinitialization only after stopping/quiescing the old session and discarding its
queued records. It is not an in-band reinterpretation of already queued tags.

| API | Contract |
| --- | --- |
| [`fvm_sbindings_init`](../include/flashvm/sbindings.h#L33) | Initialize caller descriptor/slot capacity without touching slots. No allocation/VM/GC; reinit starts an empty direction-local table. |
| [`fvm_sbindings_lookup`](../include/flashvm/sbindings.h#L37) | Copy immutable metadata to host output; UNKNOWN for zero/unbound tags. Output unchanged on failure; no object reference or authority is granted. |
| [`fvm_sbindings_payload_write`](../include/flashvm/sbindings.h#L43) | Validate all bindings/capacity then write only binary declaration payload; wrap with record_write tag 0. Caller provides separate payload and framed-output buffers. |
| [`fvm_sbindings_apply`](../include/flashvm/sbindings.h#L49) | Verify a complete tag-zero record, preflight all entries and append atomically. Copies identities; input is borrowed only during call. used publishes one consumed record only on OK. |

Registry descriptor/slots/input/output must not overlap. No hidden allocation or
locks; synchronize a shared table externally, including lookup versus apply/reset.
The implementation uses bounded two-pass validation, not a record-sized stack.
This declares schema identities only: floats/objects/audio/video still need
schema implementations and permission-aware owner-thread dispatch. It does not
automatically serialize native resources or migrate live VM objects.
## Fragmented transport input (v0.724)

[sstream.h](../include/flashvm/sstream.h) supplies an optional incremental reader.
It borrows caller storage and performs no I/O or allocation. Header bytes are
bounded/canonical before the body extent is accepted; payload fragments are
copied once and the complete CRC is calculated before publishing a view. It
accepts arbitrary socket/file/channel fragments and stops before a coalesced
second record, reporting exactly how much input was consumed.

| API | Contract |
| --- | --- |
| [`fvm_sstream_init`](../include/flashvm/sstream.h#L24) | Initialize caller state and receive storage without touching storage bytes. Capacity >=3; body limit 1..BODY_MAX. A smaller buffer enforces an additional frame bound. |
| [`fvm_sstream_push`](../include/flashvm/sstream.h#L33) | Consume input into caller storage. used reports consumed input on MORE/OK/data error; view changes only on OK. A completed view stays valid until reset/disposal. Push after completion is INVALID/used=0; data errors latch until reset. |
| [`fvm_sstream_reset`](../include/flashvm/sstream.h#L38) | Explicitly discard prefix/error/completed borrow without wiping bytes. Quiesce view users first; reset neither resynchronizes transport bytes nor cancels remote commands. |

These consumption semantics intentionally differ from complete-record read:
MORE bytes have already been retained by the stream. Resubmit only unconsumed
bytes. On OK use the view, reset, then submit any trailing input. Do not feed a
buffer into itself; state/storage/input/output must be disjoint. No concurrent
push/reset/use of a view without external synchronization. Zero input is MORE
for an incomplete prefix, not an EOF indication. The transport decides whether
EOF with retained bytes is truncation. There is no timer or hidden blocking read.
Resource/filesystem architecture / Принятый план ресурсов и ФС: [EN](RESOURCE_NAMESPACE.md) / [RU](RESOURCE_NAMESPACE.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_S_OK` | enum constant | [include/flashvm/srecord.h:30](../include/flashvm/srecord.h#L30) |
| `fvm_sbindings_apply` | function | [include/flashvm/sbindings.h:49](../include/flashvm/sbindings.h#L49) |
| `fvm_sbindings_init` | function | [include/flashvm/sbindings.h:33](../include/flashvm/sbindings.h#L33) |
| `fvm_sbindings_lookup` | function | [include/flashvm/sbindings.h:37](../include/flashvm/sbindings.h#L37) |
| `fvm_sbindings_payload_write` | function | [include/flashvm/sbindings.h:43](../include/flashvm/sbindings.h#L43) |
| `fvm_sinteger_read` | function | [include/flashvm/srecord.h:58](../include/flashvm/srecord.h#L58) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:56](../include/flashvm/srecord.h#L56) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:72](../include/flashvm/srecord.h#L72) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../include/flashvm/srecord.h#L66) |
| `fvm_sstream_init` | function | [include/flashvm/sstream.h:24](../include/flashvm/sstream.h#L24) |
| `fvm_sstream_push` | function | [include/flashvm/sstream.h:33](../include/flashvm/sstream.h#L33) |
| `fvm_sstream_reset` | function | [include/flashvm/sstream.h:38](../include/flashvm/sstream.h#L38) |
<!-- source-links:end -->
