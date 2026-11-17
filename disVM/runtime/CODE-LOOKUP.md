# Code and data lookup / Поиск кода и данных (runtime0.171)

Ordinary data retains the terminal-first launch union. Code lookup now prefers
visible execution-provider branches before terminal/other branches at the same
effective union target. This applies to each ordinary Dis `load`, including a
module's own later dependency loads. Provider branches preserve their relative
stack order, and remaining branches preserve theirs. The namespace itself and
already frozen data selections are not reordered.

Данные сохраняют обычный порядок union, изначально terminal-first. Для кода
первой просматривается provider-часть доступного стека, затем остальные слои.
Назначение запроса определяет вызывающая операция, а не расширение имени:
обычный `Sys.open("payload.dis", ...)` остаётся чтением данных.

## Explicit purpose / Явное назначение

[`intent.h`](vfs/intent.h) defines a native adapter API,
[`dvm_resource_plan_intent`](vfs/intent.c), returning the existing owned frozen
resource plan without provider I/O. Close successful plans with
[`dvm_resource_close`](vfs/resource.c). Invalid intent/arguments refuse and clear
a non-NULL empty output; never overwrite a live plan.

| Intent | Search behavior and current integration |
| --- | --- |
| `DVM_RESOURCE_DATA` | Current cwd and ordinary stack order |
| `DVM_RESOURCE_DIS` | Import base, provider-first candidates; used by the actual Dis loader |
| `DVM_RESOURCE_INTERFACE` | Same code order for interface descriptions, including `.m`; compiler/channel adapters must explicitly request it |
| `DVM_RESOURCE_NATIVE` | Same code order reserved for a future native-module adapter; no format, decoder or native code execution added |
| `DVM_RESOURCE_SWF` | Same code order for initial/nested SWF; FlashVM channel adapter remains to be connected |

These enum values are native identifiers, not assigned S-record tags or wire
messages. Intent changes lookup precedence, not permissions or execution location.
The new managed FlashVM mode is specified separately in [FLASHVM-IO-MODES.md](FLASHVM-IO-MODES.md).

The existing [`dvm_resource_plan(..., module=1, ...)`](vfs/resolution.c) and
names-only module plan use the same provider-first resolution. The
[`readmod`](vfs/loader.c) loader now explicitly requests Dis intent before source
selection and cache lookup. Builtins beginning with `$` keep their existing
registry behavior.

## Namespace, permissions and identity / Namespace, права и identity

Provider classification uses copied trusted `DVM_ORIGIN_PROVIDER` provenance,
not an address, nickname, file suffix or guess about the underlying device.
Unannotated branches stay in the remaining group in their original order.
Bind/move/copy operations keep their existing provenance propagation.

Only candidates already visible under the effective namespace binding participate.
Longer explicit binds and pinned import-directory identity keep their existing
semantics. Purpose lookup does not expose hidden provider paths, bypass grant
ceilings or grant access to the provider's complete filesystem. Ordinary data
stack inspection/moves continue to describe and edit the existing namespace order.

Strict launch unions fall through on actual absence, preserving provider denial
and transport failures. Explicit `MAUTORWX` continues to follow its existing
configured access-selection rules in the new candidate order. Actual operations
remain authoritative; no new speculative network permission protocol is added.

Provider-owned code is not downloaded for execution on the terminal by this
change. Execution stays in the selected execution environment. The existing
host loader still refuses module bytes from an unsupported provider backend;
it does not silently substitute terminal code after that refusal. The executable
test uses explicit provider-local native directory grants, not a network loader.

Код провайдера должен исполняться у провайдера. Новый порядок не реализует
удалённые вызовы module interfaces и не превращает mounted Dis в локально
исполняемый скачанный файл. `.m` описывает интерфейс; формат будущих native
модулей здесь не выбирается и не фиксируется.

## Evidence and example / Проверка и пример

The [native SDK test](../native/tests/launch/code_order.c) checks all code intents,
stable order across two providers, terminal-first data snapshots, namespace copies,
absence fallback and refusal without terminal probes after provider denial.
The node name deliberately has no code suffix; purpose is explicit.

The [actual interpreter/JIT test](fixtures/code-order/README.md) compiles conflicting
terminal/provider Dis modules and dependencies. It primes the terminal image cache
through an explicit alias, loads the provider module through the union, checks its
dependency, removes that provider branch and loads the terminal version. Previously
loaded provider code retains its identity. A data file named `payload.dis` still
reads from the terminal.
