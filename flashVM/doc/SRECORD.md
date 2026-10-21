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
fields. Tags are nonnegative 32-bit values. Zero is reserved for direction-local
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
| [`fvm_sinteger_write`](../include/flashvm/srecord.h#L50) | Borrows integer and caller output; validates 32-bit limbs/sign and full capacity before writing. OK publishes used; errors preserve bytes/used. |
| [`fvm_sinteger_read`](../include/flashvm/srecord.h#L52) | Borrows prefix; requires output value/used, publishes only on OK. MORE retains neither input nor partial result; caller retains and retries prefix. |
| [`fvm_srecord_write`](../include/flashvm/srecord.h#L60) | Borrows tag/payload, writes caller buffer only after complete validation. Payload and outputs must not overlap. No allocation, VM or transport. |
| [`fvm_srecord_read`](../include/flashvm/srecord.h#L66) | Borrows input and returns one verified borrowed view/consumed extent. limit counts body bytes. Outputs unchanged on MORE/error; trailing records remain for another call. |

All calls are reentrant with independent storage, contain no global mutable
state and do not enter VM, GC, SDL or devices. Caller serializes shared buffers.
Pointers must address valid accessible storage and output objects must not alias
each other/input. No pointer accessibility/provenance probing is performed.
[FVM_S_OK](../include/flashvm/srecord.h#L24), MORE, INVALID, LIMIT and CHECKSUM distinguish complete/incomplete,
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_S_OK` | enum constant | [include/flashvm/srecord.h:24](../include/flashvm/srecord.h#L24) |
| `fvm_sinteger_read` | function | [include/flashvm/srecord.h:52](../include/flashvm/srecord.h#L52) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:50](../include/flashvm/srecord.h#L50) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:66](../include/flashvm/srecord.h#L66) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:60](../include/flashvm/srecord.h#L60) |
<!-- source-links:end -->
