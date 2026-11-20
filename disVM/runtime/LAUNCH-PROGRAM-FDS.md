# dvm_launch_handoff_program

[Русский](LAUNCH-PROGRAM-FDS.ru.md) · [Header](vfs/launch/ticket.h) · [Example](test_unibus_files.c)

```c
int dvm_launch_handoff_program(DVM_LaunchHandoff *handoff, DVM_Provider *source,
    uint32_t pid, uint32_t parent_pid, uint32_t hidden_fds);
```

Call from the native launcher before adopting an exclusive handoff. Supply
the already authorized export root as `source`. The PID is a nonzero decimal
name in that export. A zero `parent_pid` disables inheritance. Otherwise
it selects a distinct base context in the same export. Bit N in
`hidden_fds` suppresses descriptor N. Supported descriptor numbers are 0–31.
The function can perform provider I/O. It returns 1 with both directory
capabilities retained, or 0 with the handoff unchanged. Release the handoff
through `dvm_launch_handoff_free`. The PID identifies a name inside the
granted root and grants no authority by itself.

Adoption reads `.fd` in each retained directory. The selected context's entry
overrides the base entry of the same name. Explicit omissions suppress both.
The receiver opens each resulting file once, before entering guest code.
The mode comes from the file's advertised read/write bits. A scoped launch
with an absent standard descriptor has no host standard-stream fallback.
Already opened files pass to local `spawn` descendants with their shared
cursor. A native publisher can preserve snapshot semantics by publishing
retained handles whose bindings stay stable after context capture.

Run `ctest -R '^terminal-ordered-files-guest$' -V` in a configured SDK.
The fixture selects `.prog/42` above `.prog/41`, overrides FD 1, hides FD 2
and inherits FD 0 and 3. The guest reads FD 3 before and after a child reads
it, proving the shared cursor. The test checks file bytes for both the
unused base FD 1 and the hidden FD 2. This API imports descriptor contexts
from a trusted host selection. Process advertisement, launch-wire PID
selection and environment groups are outside this entry point.
