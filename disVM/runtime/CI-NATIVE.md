# Native architecture CI completion

Copyright (c) 2026 bhgv

[Русский](CI-NATIVE.ru.md) · [Workflow](../../.github/workflows/native-disvm.yml)

The native workflow lets a running master matrix finish when another commit is
published. RV64/QEMU may take longer than one development step; cancelling on each
push repeatedly prevented a complete architecture result.

The branch concurrency group retains one running matrix and the latest pending
work. GitHub may replace an older pending run with the newer one. Running master
work is not cancelled by this policy; other refs retain cancellation. Queueing is
not a successful check: inspect the result for the exact commit being reviewed.

Workload selection, timeouts, sanitizer settings, native execution profile and
success criteria are unchanged. Native OS owners without global stop/exclusive
remain mandatory. RV64 userspace/QEMU execution is not a bare-metal/whole OS claim.

The policy change was parsed as YAML and checked to preserve every job and the
existing branch concurrency group. Its eventual execution/queue status is reported
separately from local runtime tests.
