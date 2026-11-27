# fvm_debug_step_action

[Русский](DEBUG_STEP_ACTION.ru.md) · [Debugger API](DEBUG_API.md) · [Implementation](../src/jit/breakpoint.cpp)

```c
#include <flashvm/debug.h>
FVM_Result fvm_debug_step_action(FVM_Context *context);
```

Call from the active synchronous breakpoint handler on the context owner.
`context` is borrowed. A successful call returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) and arms one
stop before the next interpreted action, including an end action. The
current handler must return [`FVM_OK`](../include/flashvm/flashvm.h#L44) to continue toward that boundary.
A handler failure or replacement clears the pending step. Outside the
handler the call returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). It does not advance execution
or switch the execution engine by itself. The
[paired player](STYX_RPC_DEBUG.md) switches to interpretation before
attaching its debugger and uses this request for `.state step`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:259](../src/core/runtime.hpp#L259) |
| `fvm_debug_step_action` | function | [include/flashvm/debug.h:74](../include/flashvm/debug.h#L74) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
