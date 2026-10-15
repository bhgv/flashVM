# Native class Actions — v0.360

[Русское описание](JIT_CLASSES.ru.md)

Extends, ImplementsOp, InstanceOf and CastOp now lower to direct native helpers
using the shared AVM1 inheritance/interface/membership services. Extends retains
subclass then superclass on the stack while the service performs its own
superclass-before-subclass boxing. ImplementsOp converts its count before copying
reversed interface operands, retaining the constructor and all inputs until the
service publishes its edges. CastOp preserves its distinct class/value order;
InstanceOf consumes value/class. Successful matching results replace a rooted
operand before stack shrink without allocating after service roots are released.

Existing version policies remain shared with interpretation: host constructor
edges differ from loaded SWF7+ interface prototype snapshots; zero loaded counts
are noops, object counts do not invoke valueOf, and an installed loaded snapshot
list is not replaced. Matching uses the service's own stored prototype policy.
Loaded primitive Cast performs boxing side effects but returns null rather than
the wrapper, propagating constructor errors. Callback/property side effects are
not rolled back on failure.

| Test | Coverage |
| --- | --- |
| `jit_classes` | Isolated host/SWF4–8 environments for interpretation and explicit/automatic native bodies; inherited property, positive membership/cast and negative cast, interfaces/SWF7 gating, changed interface prototype versus retained snapshot alias, missing operands and restoration |
| `jit_classes_callbacks` | Collecting/throwing superclass getter that deletes itself; detached parent retained into subclass setter; constructor metadata; publication despite a setter exception; collecting/throwing primitive Cast boxing |
| `jit_classes_allocation` | Host/loaded invocation allocation failures across all four Actions, with restored frames/depths/active code and post-failure GC |

Function definitions, With/Try and other remaining Actions still require
interpretation. This extends the native subset without completing Flash class
semantics or any roadmap stage; foreign CPU/OS acceptance remains open.

Linux Release: all 732 AGG, 732 Cairo and 713 minimal checks pass after restoring
generated ELF execute permissions. Three class tests pass restricted heap/global
ASan+UBSan; stack ASan and LeakSanitizer remain disabled.
