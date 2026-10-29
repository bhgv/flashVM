# Native library compiler jobs on Dis — runtime 0.33

[Русский](COMPILER-SERVICE.ru.md) · [Workspace](WORKSPACE.md) · [Worker interface](dis/bridge/idecompiler.m) · [Native interface](../disVM/module/fircompiler.m) · [C99 service](../disVM/compiler/compiler_engine.h) · [Tutorial](dis/examples/compiler-service/README.md)

The IDE has an executable build-service route: a Limbo worker snapshots a real
workspace document, invokes our Coco/R/FIR libraries through a typed native module,
and submits the actual artifact with its original ticket/revision. Dis still owns
document, job and publication policy. The native extension has no UI authority.
No shell, Python, external Inferno emulator or compiler CLI process implements a
native compile call. Python is development acceptance tooling only.

This is a service foundation. The graphical Build/Run/Debug panels, automatic
artifact loading/execution and full debugger connection remain unfinished.
The bounded source compiler does not yet compile the complete Limbo IDE itself;
complex IDE modules are still bootstrapped from the pinned compiler source.

## Three interfaces and ownership

| Layer | Interface | Responsibility |
| --- | --- | --- |
| Host library | dvm_compile / dvm_compile_dispose | Source → verified FIR → optional optimization → complete artifact |
| Native Dis module | $Fircompiler.compile | Capture matched NC64 frame, validate options, copy native bytes into rooted Dis results |
| Limbo worker | idecompiler.start → request channel | BeginBuild snapshot, native call, FinishBuild admission, unique reply and shutdown |

The C99 library is independent of VM headers. Inputs are borrowed immutable UTF-8
bytes. Every stream, FIR module and linker context is private; success owns exact
artifact/debug byte buffers, and failure exposes neither partial output. Dispose
both buffers exactly once (repeated disposal is safe). Reusing an output object
requires disposal before another compile. Calls are serialized because the source
parser has process-global state. No source parser has been added outside Coco/R.

The native boundary captures the generated frame before callbacks. Packed result
slots use byte copies. Previous mapped roots are validated together and cleared
before destruction. Each new array is rooted before the next allocation; protected
publication releases all C buffers even if Dis allocation raises. Inputs remain
on the live caller stack during compilation. No C buffer, heap pointer or native
layout crosses a module/channel wire boundary. Byte arrays may use canonical or
pointer-free one-byte module-local descriptors; descriptor identity alone does
not establish their primitive layout in NC64.

The worker retains loaded modules and the borrowed workspace endpoint on its
server stack. It owns no document and never shuts down the workspace. Shutdown
is FIFO and terminal: consume its acknowledgement and do not send again. Requests
without a reply endpoint are ignored; valid endpoints receive exactly one reply.
A missing native module makes start return nil. No fallback process is launched.

## Formats, budgets and diagnostics

Languages 1..4 are AS1, AS2, AS3 and Limbo. They retain their existing executable
coverage; AS3 here is AVM1 compatibility, not ABC/AVM2. Targets are 1 Dis, 2 FIR,
3 SWF. optimize must be exactly zero or one. Wider NC64 integers are validated
before conversion into host C int.

FIR output is canonical. Dis uses the existing NC64 scalar/hosted backend. SWF is
a complete SWF8 file produced from a private SWO through libswl_core with stage
640×480, 12 fps and extensions disabled. Target-incompatible operations fail
explicitly; no partial SWO is mislabeled SWF. The same CLI options reproduce the
service's bytes. Native compilation does not execute or publish these artifacts.

Source is limited to 65536 UTF-8 bytes. Artifact and original FIRDBG1 independently
allow at most 2097152 bytes. Workspace adds its existing 2 MiB per artifact and
8 MiB aggregate limits. Temporary streams use the existing secure FIR staging
policy; an unavailable temporary directory is a diagnosed failure. The caller
chooses whether and where returned bytes are saved. The tutorial's Sys.create
is an explicit client action, separate from service authority.

| Stage | Meaning |
| --- | --- |
| Ok (0) | Complete artifact and original debug bytes |
| Invalid (1) | Invalid option/input representation |
| Source (2) | Source-library rejection; inspect diagnostic kind and location |
| Verify (3) | FIR reader/verifier/optimizer rejection |
| Backend (4) | Dis, object or linker rejection |
| Resource (5) | Native allocation/staging/output failure |
| Limit (6) | Source or output service budget exceeded |

The native tuple also returns plain message, public source diagnostic kind,
one-based line, zero-based UTF-8 byte offset and byte length. Absent location is
line=0, offset=-1, length=0. EOF locations may have zero length. Debug is the
original source/unoptimized FIR provenance, unchanged across target/optimization;
it is not a final Dis/SWF/native-PC debugger map. The module name is ide_build.

The worker Reply.status is a workspace status; Reply.compiler is the native
stage or -1 if no compilation ran. A failed native compile is submitted through
FinishBuild and normally returns BuildFailed. A newer ticket or changed source
can instead produce Conflict. Debug is returned only after admitted success.
Artifact bytes are fetched separately through workspace.Artifact, which copies
again. A failed job preserves the previous successful artifact only while its
source revision is current. A result from an old real snapshot cannot replace a
newer document, even if its compilation succeeded.

Compilation is synchronous and blocks the sole VM execution owner. There is no
cancellation, parallel parser or responsiveness guarantee during a native call.
Workspace admission handles revision changes between job phases; the acceptance
case explicitly compiles an old snapshot and then rejects its late real bytes.
It does not pretend edits ran concurrently inside a blocking native call.

## Build and package

Root graphical builds enable the service after building the matching FIR/linker:

```sh
./build-workspace.sh --projects fir,linker,disvm-editor --jobs 4
```

Runtime-only root builds enable it when both fir and linker are selected. A
standalone runtime requires DVM_RUNTIME_COMPILER=ON and four explicit matching
paths: DVM_COMPILER_FIR_INCLUDE, DVM_COMPILER_FIR_LIB,
DVM_COMPILER_SWL_INCLUDE, DVM_COMPILER_SWL_LIB. Library directories contain
libfir1_source_api.a, libfir1_ir.a, libfir1_io.a and libswl_core.a. No build fetches
an unknown toolchain or silently compiles dependencies from sibling directories.
The C99 service can also be built independently with the same four paths:

```sh
cmake -S disVM/compiler -B build/compiler-service -DDVM_COMPILER_TESTS=ON \
  -DDVM_COMPILER_FIR_INCLUDE="$PWD/fir/v1/include" -DDVM_COMPILER_FIR_LIB="$PWD/build/fir" \
  -DDVM_COMPILER_SWL_INCLUDE="$PWD/linker/include" -DDVM_COMPILER_SWL_LIB="$PWD/build/linker"
cmake --build build/compiler-service
ctest --test-dir build/compiler-service --output-on-failure
```

Packages record Fircompiler=ON/OFF in NATIVE-MODULES.txt. Enabled packages ship
compiler-service.dis/.sbl, idecompiler.dis/.sbl, interfaces, source, documentation
and the service archive. Disabling the capability removes installer-owned stale
tutorial/archive products. The ordinary worker module remains loadable and its
start reports unavailable. No deployed CLI binary is needed by the builtin.

## Acceptance

The tutorial creates 24 real artifacts: all four language profiles, both
optimization choices and all three formats. Host acceptance independently runs
CLI libraries, compares every artifact byte, compares complete source debug JSON,
executes every produced Dis and linked SWF, and checks exact result 42. Interpreter
and JIT run with quanta 1 and 2048; statistics distinguish dispatch and GC.

Actual workspace/worker channels test successful admission, a stale request that
never compiles, a failed new job retaining prior bytes, target changes, independent
artifact copies, rejection of late real compilation after an edit, and terminal
shutdown. The C99 fixture alternates 24 successful owned results with 32 clean
failures, tests double disposal and unavailable temporary staging. Relocated SDK
checks relink the C99 client against shipped archives and rerun native builds
without references to the original checkout. Physical ARM64/RV64, full IDE
self-hosting and final debugger attachment remain separate acceptance work.

The shared Math fix is published upstream and the host-only rewrite is retired.
Checked fetch precedes GC/quota; direct dispatch uses captured callback pointers.
See the pinned [synchronization record](../disVM/SYNC.md).
