# Guarded scalar constructors — runtime0.116

Selected hosted CONSB/W/L/F now share `scalar_cons.c` in C and SysV x64 JIT.
The source scalar is copied once under its exact span/range guard into zeroed
private native storage. Byte heads retain the original IBY2WD padded allocation;
WORD/LONG/REAL heads retain their actual native widths. No numeric conversion
occurs: negative zero, infinity and NaN payload bits are preserved.

Under ACTIVE/NativeWorld the coordinator captures the current Tlist/H destination
hold, uses `value_prepare.c`, and conditionally commits using `heap_commit.c`.
A stale tail destroys candidate/observation and fully rebuilds, at most eight
preparations; the original scalar snapshot remains fixed even for overlapping
source/destination. Complete candidate/retired/tail cleanup continues across
errors, preserving the primary body error or first retirement error. A cleanup
error after commit does not undo publication. Deferred candidates remain rooted
until Fast. No allocation, monitor, consuming destruction or VM unwind occurs
under an object guard.

Caller supplies mapped owning operand cells, rooted frames/containers, stable
Type/frame/code/cell lifetime and no held guards. Neither pointer geometry nor
reference counts prove those premises. NativeWorld and the existing serialized
allocator/collector remain required. Unselected and serial-inline C modes return
a pre-effect decline to their original opcode bodies.

## C and JIT routing

`prepare_scalar_cons.py` pins exact adapted C bodies and the original scalar
emitter with SHA256. It rejects drift/double adaptation. Selected C opcodes call
the coordinator; unselected code retains the old bodies. Selected x64 emission
checks the actual aggregate/REG/Heap profile and scalar allocation width, then
emits a tail JMP to one of three checked1/4/8-byte C entries in `scalar_tail.c`.
The old scalar machine-code allocator/copy path remains only for unselected mode.

The tail entry rejects foreign/inactive/guarded REG before reading operands.
No RX return address or borrowed REG/cache metadata is used after a callback
can retire the current JIT mapping. Errors propagate across the ordinary C stack;
there is no decline/retry after effects. A four-byte helper is compiled, but this
native WORD64 checkout exercises actual byte and eight-byte opcode profiles;
this is not native WORD32/platform acceptance.

## Reproduction and example

`test_scalar_cons.c` is a complete commented native worked fixture. Eight cases
cover ordinary creation, H tail, source/destination alias, one/repeated stale
writes, alias+stale source snapshot, primary plus secondary cleanup errors, and
successful publication followed by multiple retirement errors. Six refusals
cover four root-reservation allocations, node allocation and Tlist retain.
Real containers, Strings, actual Cycles/Fast and Type baselines are used.
Separate exact-bit cases preserve binary64 negative zero, infinity and NaN payload.
The Limbo guest builds byte/int/big/real lists through actual C/JIT dispatch.
Three scoped source controls detect missing stale cleanup, incomplete unwind and
primary-error overwrite with semantic exit10 before unsafe continuation.

```
python3 disVM/runtime/test_scalar_cons.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
python3 disVM/runtime/test_scalar_tail.py \
  build/runtime/disvm build/install1804/disvm-runtime/limbo-native disVM/upstream/module
```

The second fixture checks the actual eight-byte x64 target: foreign REG/cold
thread, current code unmap, VM error, combined unmap/error and strict bad-CALL
control. Other width entries share checked invocation but do not inherit that
fixture's full mapping-retirement proof automatically. TSan covers native C;
no machine-code TSan, GUI, Android, ARM64 or RV64 claim follows.

CONSM/CONSMP are not switched: arbitrary raw snapshots and mapped reference
ownership need further work. This is four scalar opcodes, not general parallel
managed execution or a new serialized instruction/ABI. Known additional paths
are marked SCALARCONS-116 in [the cost ledger](PERFORMANCE-COSTS.md).
