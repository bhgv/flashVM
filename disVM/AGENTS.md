# DisVM development rules

- This is the independent Dis interpreter/JIT engine, complementary to root flashVM/.
  Keep heaps, GC roots, scheduling and native APIs separate. Interoperation uses
  checked typed module/channel boundaries, never cross-heap native pointers.
- Upstream is https://github.com/bhgv/Inferno-OS-bhgv, branch experiment.
  UPSTREAM.json pins selection, revision and hashes. Keep upstream/ byte-for-byte;
  confirmed defects are fixed/tested/exported in the owning upstream repository,
  then refreshed with tools/vendor-sync.py. Host adapters stay outside upstream/.
- Preserve component notices and LGPL/GPL texts. Do not relabel upstream as MIT.
- Retain x86/i386 reference source; prioritize x86-64, arm64 and riscv64. Do not
  equate leaf-JIT/native fixture execution with a complete VM/platform/JIT port.
- No Draw/Tk/Prefab/Keyring implementation or whole OS import. Retained declarations
  and selected OS boundary bodies used by tests are not a GUI/runtime dependency.
- Document every handwritten type/function and ownership boundary. GC comments
  begin [GC:]. Keep public bridge formats scalar/value/capability based.
- Build/test independently from this directory using explicit host inputs.
  Do not create implicit dependencies on sibling source trees or external SDK binaries.
- Prepare/test local work branches and retain format-patches. The user has
  authorized direct GitHub publication to bhgv/flashVM. Check the expected
  master head and resulting tree; never force-push. Upstream fixes remain
  separate changes in their owning repository. Local author/committer:
  bhgv <bhgv@bhgv.bhgv>.

## Dedicated Dis execution

- In the dedicated model, bind exactly one `Prog` and one live register set
  to each execution thread at startup. Keep that identity and register set
  until the process terminates. Never pick another `Prog` from a ready queue
  or reload the live registers from `Prog.R` on wake, channel completion,
  native return, or the next dispatch. A saved image for GC or diagnostics is
  a publication from the owner, not the source for resuming the thread.
- Send cross-thread requests and completions through the resident's mailbox.
  Test this contract with `--native-spawn`. The shared executor pool is removed;
  do not revive pool/migration scenarios or assert successful execution with
  `--managed-workers N` for N > 1, `auto`, or `--managed-workers-limit`.
  Test those forms only for explicit CLI rejection before VM startup.
  `--managed-workers` (including 1), `--gc-inline` and `--sequential-ui` are
  removed execution modes; test them only for rejection before VM startup.
- Active development and publication use `master`. Native OS threads with no
  global exclusive/stop are mandatory for all supported hosts and diagnostics.
  Do not use single-threaded/exclusive reference profiles in new work or CI.
  Historical source/fixtures do not authorize restoring those execution modes.

## JIT diagnostic scope / Границы диагностики JIT

- Cover instructions and mechanisms actually used by the supported hosted JIT.
  Inspect the final generated backend and its mandatory guarded dispatch, not
  only retained upstream emitters or the guest opcode name. A native bridge to
  C does not prove inline machine arithmetic. Test-injected RX code demonstrates
  fault handling, not production emission of those instructions.
- Do not add CPU extension state, decoders, debugger commands or recovery paths
  for mechanisms the JIT does not use. Extend diagnostics alongside an actual
  backend change that needs them. Preserve existing tested snapshots; their
  availability does not require expanding the supported instruction set.
- Диагностика покрывает фактически используемый код hosted JIT. Проверять итог
  генерации и обязательный guarded dispatch. Переход в C и тестовая RX-подмена
  не доказывают inline-арифметику. Не добавлять неиспользуемые механизмы CPU,
  команды отладчика и пути восстановления ради возможной будущей поддержки.
