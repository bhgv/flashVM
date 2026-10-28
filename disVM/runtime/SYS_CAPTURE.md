# Captured Sys boundaries and result ownership

[Русский](SYS_CAPTURE.ru.md) · [Runtime](README.md) ·
[Internal contracts](sys_capture.h) · [Guard implementation](sys_capture.c) ·
[Real-header tests](test_sys_capture.c) · [File tutorial](../examples/filesystem/README.md)

The selected Inferno source is pinned to
[9ee638bb](https://github.com/bhgv/Inferno-OS-bhgv/commit/9ee638bbfaff421164e2e7fb1d68e01089720c39).
Six incoming commits since 9a87d74 capture Sys operands/results across allocation,
finalization and native callbacks, check native-width narrowing, and preflight
aggregate owners. Our additional pipe element-layout fix was published back to
Inferno's experiment branch, then imported byte for byte through UPSTREAM.json.
The selected source tests accompany the code; builds never fetch moving HEAD.

## Independent host adaptation

The optional NC64 runtime supplies its own bounded file/stdio/metadata adapters;
it does not compile Inferno's complete Sys OS implementation. Importing fixed
upstream code alone therefore would not fix those host functions. The new guards
adapt sysstatusframe/sysresultowner principles from upstream 86a2bf2:

| Host functions | Captured boundary |
| --- | --- |
| Sys.open, Sys.create, Sys.fildes | Aligned fixed frame, complete original owner slot, previous referenced owner before conversion/allocation. Publish to that same slot; retire the captured old owner. |
| Sys.read, Sys.readn, Sys.write | Aligned frame/result; checked native descriptor and copied byte-array header. Count, data and FD remain the same across a multi-fragment read. Reject counts beyond signed native int before narrowing. |
| Sys.seek | Aligned frame and original big result; existing descriptor/origin/native-offset limits remain explicit. |
| Sys.print, Sys.fprint | Captured fixed prefix and result; formatted arguments still come from the original variadic area. Capture the numeric fprint FD before formatting. Output uses the existing stdio/file ports. |
| Sys.stat, Sys.fstat | Complete status/Dir result span; all four old string owners and alias multiplicities validated before any root is cleared or destroyed. |
| Sys.dirread | Complete status/array span and previous array owner checked; original FD captured before old-result destruction. Native batch cleanup still uses the protected mapped builder. |

`dvm_sys_capture` copies with memmove so valid packed frame/result storage is not
read through an unaligned C lvalue. It rejects nil/H or wrapping frame/result
spans before callbacks. The metadata result check covers the entire tuple, not
only its leading status WORD. Root publication, rollback and error propagation
remain in the calling adapter.

`dvm_sys_owner` reads a previous owner without changing its reference count.
`dvm_sys_owners` checks how many references aliasing result slots will consume
before the first destroy. A record with two slots naming the same object needs at
least two owned references. All previous owners are captured before clearing the
record, so no partial destruction occurs for an already invalid alias count.

These helpers are not a heap registry or memory sandbox. They assume VM-owned
mapped storage and valid managed pointer provenance. Original argument/result
roots must remain live through the call; the local C frame copy is **not a GC
root** and does not authorize arbitrary cross-thread heap access. The runtime
still has one Dis execution/GC owner. Ports must not reenter or mutate that VM.
Raw native addresses are not channel capabilities. File and directory OS code
remains under the selected FlashVM ports; no Linux-specific syscall was added to
these adapters.

## Upstream pipe correction

Inferno Sys.pipe previously inspected pointer-sized old owners after checking
array length/data only. It now also checks that the element descriptor has pointer
size, one map byte and leading pointer bit 0x80 before inspecting any element.
Equivalent independent pointer descriptors are accepted; byte/scalar arrays,
nil/H and low/wrapping descriptors are rejected without native pipe creation.
The focused upstream regression fails on the parent source and passes in emu/os,
WORD32/native profiles (28 cases). Shared owner/capture matrices still pass.

This snapshot does **not** enable native Sys.pipe or network operations in our
NC64 host. Their implementation, cancellation and OS-port contracts remain open.
No Flash-specific any/table/projection extensions were exported to Inferno.
There were no new JIT backend changes in these incoming six commits.

## Tests and limits

- Actual matched NC64 headers: every packed frame/result offset, original-result
  publication after argument clobber, nil/H/wrap rejection, zero/alias heap refs,
  and wrapped complete stat/fstat/dirread tuples whose leading WORD would fit.
- Selected upstream Sys tests cover both emu/os and native/WORD32 models. Imported
  CTest FD-owner capture runs bound frame/array-shift dimensions to zero while retaining
  callback phases and other cases; they are not the exhaustive upstream matrix.
- Existing runtime tests execute real compiled Limbo modules, GC, files, standard
  libraries and IDE in interpreter/native-JIT modes. ABI/source checks do not
  establish a complete compiler, full native Sys or physical platform acceptance.
