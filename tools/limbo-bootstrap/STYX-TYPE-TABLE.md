# Compile Styx type metadata into a Dis image

[Русский](STYX-TYPE-TABLE.ru.md) · [Compiler hook](host/styx_export.c) · [Loader](../../disVM/runtime/vfs/module_shapes.c)

Use the CMake-built hosted Limbo bootstrap with
`LIMBO_BOOTSTRAP_CELL_BYTES=8`. Ordinary Dis compilation retains the
resolved argument and result types for exports and imports by default. For example, from the
repository root with an existing hosted build:

```sh
build/scalar-runtime/bootstrap/limbo-bootstrap \
  -I disVM/examples/remote-scalar \
  -o build/blend-owner.dis disVM/examples/remote-scalar/blend-owner.b
```

The compiler retains the existing export name and 32-bit Dis signature.
It writes the type table after the legacy source-path terminator, using
the types already resolved from the implementation and its included
interfaces. The CMake four-byte target and the shell bootstrap emit
legacy images. Hosted DisVM also loads previously compiled images
without the table.

The table describes `int`, `big`, nested tuples and references to
untagged, non-polymorphic ADTs with scalar `int` and `big` data fields.
Each shape has at most 48 bytes and each value has at most six levels of
types. Unsupported exports or shapes beyond those bounds are omitted.
The table body is at most 65536 bytes. If the next record would exceed
that bound, the compiler ends the table at the previous record. The export-only marker is `STYXCS1!`. It is followed by
records in export order and a four-byte `0xffffffff` terminator. Each
record contains a big-endian 32-bit export index and signature, the two
one-byte shape lengths, then the two counted ASCII shapes. The index is
zero-based in the existing export table.

The hosted loader reads both tables from the same opened image. It rejects
a recognized malformed table or an export index or signature that does
not match that image. See the [published profile format](../../disVM/native/terminal/UNIBUS-SERVER-LOAD-PROFILE.md#compiled-types)
and [two-VM example](../../disVM/examples/remote-scalar/STYX.md#call-with-three-arguments).


## Import types

An image with a Dis linkage descriptor table uses the marker
`STYXCS2!`. It contains the same terminated export section followed
by an import section. Each import record starts with a big-endian
32-bit linkage group number, followed by the same record structure as
an export. In the import section the index identifies the slot inside
the group. Both numbers are zero-based in the existing Dis linkage
table. Records are sorted by group and slot. Unsupported shapes can be
omitted. A group number of `0xffffffff` ends the import section and the
image. Each section has its own 65536-byte limit.

The loader checks import slots and signatures against the decoded
linkage table before admitting the image. For a mounted `load`, the
client compares the shape of the exact local import with the shape
offered by the provider when both are present. Identical name and
32-bit signature remain required. Existing images without import
shapes still use the original check for the existing gateway profiles.
See the [two-VM mismatch case](../../disVM/examples/remote-scalar/STYX.md#call-with-three-arguments).
