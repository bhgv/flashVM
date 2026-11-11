# Private host runtime source modules

[main.c](../main.c) composes these private implementation fragments in their
original dependency order. They intentionally remain one C translation unit, so
existing static state, forward declarations and callback visibility keep their
semantics. Each handwritten fragment stays below 500 lines. The initial split
preserved the complete ordered source bytes before the publication fixes were
applied; subsequent formatting checked native tokens and preprocessor boundaries.
No preprocessor conditional spans two fragments.

| Module | Responsibility |
| --- | --- |
| [host_state.inc](host_state.inc) | Includes, host state, counters and initial diagnostics |
| [diagnostics_and_files.inc](diagnostics_and_files.inc) | Inspection, native files and cancellation readiness |
| [cancellations_and_stdio.inc](cancellations_and_stdio.inc) | Host cancellation delivery and standard-I/O helpers |
| [input_and_arena.inc](input_and_arena.inc) | Native input, channel queue guards and allocator startup |
| [errors_and_spawn.inc](errors_and_spawn.inc) | Owner-local errors, process allocation and native spawn |
| [channel_claims.inc](channel_claims.inc) | Channel completion claims and cancellation preparation |
| [process_control.inc](process_control.inc) | Wait replies, child lifecycle and termination checkpoints |
| [channel_retirement.inc](channel_retirement.inc) | Channel shutdown, detached waits and native finalization |
| [modules_and_boot.inc](modules_and_boot.inc) | Builtins, loading and initial module startup |
| [owner_requests.inc](owner_requests.inc) | Owner mailbox delivery and retry admission |
| [dispatch.inc](dispatch.inc) | Permanent resident dispatch, error handling and retirement |
| [execution.inc](execution.inc) | Runtime startup, main execution and joined shutdown |
| [options.inc](options.inc) | CLI parsing and explicit profile validation |
| [system_entries.inc](system_entries.inc) | Remaining hosted system entry points |

These fragments are not independently linked libraries. Build the component with
its normal CMake target, which compiles main.c and tracks included fragments as
compiler dependencies. Copying main.c alone is not a source distribution: retain
this directory. Public formats and imported Inferno snapshots remain unchanged.

Private fragments retain historical comments that have not all received a new
semantic contract audit; splitting and formatting do not certify those contracts.
Current ownership/publication behavior is documented in
[RUNQUEUE-PUBLICATION.md](../RUNQUEUE-PUBLICATION.md).
