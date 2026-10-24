# IDE build backend changes / Изменения

- 0.1.1: Add independent transactional IDE build backend.
- 0.1.2: Cover IDE build success.
- 0.1.3: Cover IDE build source failure.
- 0.1.4: Cover IDE build verify failure.
- 0.1.5: Cover IDE build optimise failure.
- 0.1.6: Cover IDE build verify optimised failure.
- 0.1.7: Cover IDE build object failure.
- 0.1.8: Cover IDE build link failure.
- 0.1.9: Cover IDE build run failure.
- 0.1.10: Cover IDE build trace.
- 0.1.11: Cover IDE build timeout.
- 0.1.12: Cover IDE build aliases.
- 0.1.13: Cover IDE build invalid requests.
- 0.1.14: Cover IDE build missing artifacts.
- 0.1.15: Cover IDE build invalid artifacts.
- 0.1.16: Cover IDE build unicode paths.
- 0.1.17: Cover IDE build cli.
- 0.1.18: Cover IDE build cleanup.
- 0.1.19: Add real numeric build example and record toolchain acceptance.
- 0.1.20: Adopt Limbo/Dis-hosted IDE and channel-only FlashVM UI architecture.
- 0.1.21: Require human-readable commented IDE/runtime bridge code; format host tools and tests without changing Python semantics.
- 0.1.22: Specify bidirectional AS1/AS2-Dis module interfaces and mixed-language IDE requirements.
  Clarification: new JIT targets are x86-64, arm64 and riscv64; existing x86 is retained.

- 0.2.0–0.2.7: Introduce the actual Limbo/Dis document core, typed channel/session
  semantics, interpreter/reference-mode regressions, Unicode/bounds/concurrency
  cases, revision guards, checked module/debug outputs, production-only install,
  source-map reader acceptance and explicit bytecode delivery requirements.

- 0.2.8: Reject arbitrary module bytes and normalized-away symbol header corruption.
  Check unsigned Dis XMAGIC and exact LF/CRLF physical symbol prefixes with bounded
  reads; expand host admission regressions from six to 46 cases. No Dis/JIT runtime
  or full bytecode verification claim is added.

- 0.3.0: Add revision-bound character offset/line/column queries to the Limbo core.
  Reply gains three fields; all clients must be rebuilt. Preserve state and
  round-trip every LF/Unicode/CRLF character boundary through typed channels.

- 0.3.1: Add exact bounded Unicode-character Find through the document channel,
  with revision checks, explicit NotFound and a linear-time private prefix table.
