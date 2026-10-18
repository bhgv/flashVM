# Resource exceptions and AVM1 handlers (v0.8)

FlashVM now executes SWF 7–8 `ActionTry` (0x8F) and `ActionThrow` (0x2A).
Use ordinary ActionScript `try { ... } catch (failure) { ... } finally { ... }`.
A new `try:/except:` grammar is unnecessary. This repository does not include an
ActionScript source compiler. `examples/actionscript/resource_exceptions.as` is
source documentation; executable bytecode tests construct the actual records.

Catch may bind a name or a register. Nested handlers share their function's
operand stack, registers, constant pool and instruction budget. Throws propagate
through script and native calls. A normal finally preserves a pending throw or
return; a finally throw/return supersedes it. Catch bindings are restored after
the handler. Values saved for finally and the pending exception are GC roots.
The inspector recursively checks Try block framing before top-level execution;
blocks are sized by their header and do not need End. Nesting is limited to 64.
Cross-block jumps and DefineFunction2/register banks beyond the existing four
registers are not implemented. Invalid bytecode, allocation failures and
unsupported instructions remain VM statuses rather than catchable AS throws.
Existing DefineFunction record framing retains its previous compatibility format.
This is not a certification of complete Flash 8 execution.

## ResourceException

Failing AS mutex/channel methods and getters return [`FVM_EXCEPTION`](../include/flashvm/flashvm.h#L48) to the host
and throw a plain object with `name`, numeric `code`, stable English `message`
and an object named `context`. Match the code rather than localized text.

| Code | Message prefix | Meaning |
| --- | --- | --- |
| 1001 | resource.closed | Explicitly closed resource |
| 1002 | resource.stale | Invalid authority session or resource token |
| 1003 | resource.disconnected | Established connection lost, EOF or timeout |
| 1004 | resource.unreachable | TCP connection could not be established |
| 1005 | resource.not_owner | Reset attempted by another context |
| 1006 | resource.recursive | Repeated nonrecursive acquisition |
| 1007 | resource.cancelled | Cancelled resource wait |
| 1008 | resource.io | File I/O failure, including blocking read at EOF |
| 1009 | resource.failure | Other resource failure |

Context fields: `resourceKind`, `resource`, `operation`, `threadId`,
`originThreadId`, `transport`, `flags`, `path`, `address`, `source`, `function`,
`line`, `offset`. Transport flags can combine thread (1), file (2), network (4).
The transport string selects file, then network, then thread, otherwise local.
Origin IDs describe a live context when known; zero means unknown/remote.
Network proxy resource IDs are local diagnostic IDs, not global network IDs.
File ingress preserves its path on imported capabilities. Paths/addresses in
C debugger snapshots are bounded (511 bytes plus terminator), source/function
255 bytes. Existing registered source maps supply the source position; an
unmapped position has an action offset and otherwise empty/default metadata.

The resource authority transmits stable failure codes in the existing response
status field. Older clients see a generic failure. Older authorities supply only
a generic failure; clients cannot infer a more specific cause. No retry follows
a lost mutex acquisition response: the owner may have acquired the mutex, so
its outcome is uncertain. Leases, lost-ACK reconciliation and TLS remain absent.

## C embedding and debugger

`include/flashvm/exception.h` exposes `get`, `info`, `clear`, `raise`.
After [`FVM_EXCEPTION`](../include/flashvm/flashvm.h#L48), obtain the pending value with [`fvm_exception_get`](../include/flashvm/exception.h#L61) and
inspect it before [`fvm_exception_clear`](../include/flashvm/exception.h#L65). Borrowed object values lose their
exception root on clear; publish or pin a value if retaining it. `raise` accepts
any context-owned AS value and returns [`FVM_EXCEPTION`](../include/flashvm/flashvm.h#L48). Strings are copied.
`info` returns NOT_FOUND for ordinary script throws. Low-level native resource
APIs preserve their existing [FVM_ERROR](../include/flashvm/flashvm.h#L43) contract; contextual faults can be read
with `info`/`get`. The AS method/getter bridge materializes and throws them.
A native worker callback returning [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48) reports that status through
join; its context-owned exception object is not automatically transferred to the
joining context. Handle/send the exception inside the worker when data is needed.

[`fvm_debug_exception_events`](../include/flashvm/exception.h#L70) provides a process-wide ring of 64 copied resource
exception snapshots. Overflow drops the oldest event. Insufficient capacity
returns NOT_FOUND and the required count without clearing. Events remain after
AS catch/clear. Ordinary script throws do not enter this resource event ring.
Legacy [`fvm_debug_resource_errors`](../include/flashvm/debug.h#L156) remains available. These APIs supply diagnostic
interception, not an interactive debugger UI, execution suspension or a remote
queue-freeze protocol.

## Executable coverage

- `tests/actionscript_exceptions.cpp`: named/register catch, nesting, rethrow,
  finally on return, finally override, native method and script-call propagation,
  malformed embedded framing, pending exception GC and collection inside finally.
- `tests/resource_exception_network.c`: real loopback TCP response loss versus
  connection refusal, stale authority session, stable code/message context and address preservation.
- `tests/resource_exception_file.c`: closed mutex restored from a file packet,
  offending resource identity, file/network provenance and copied debugger event.
- Existing broker, independent-process, worker and file tests check the updated
  AS exception status while preserving low-level native status expectations.

The public change is additive: a new result enum and a separate exception header;
existing public struct layouts and [FVM_ABI_VERSION](../include/flashvm/flashvm.h#L25) remain unchanged.

Validation results: [v0.8 report](EXCEPTIONS_REPORT.md).

## v0.9 Error integration

Materialized resource exceptions now expose native Error.toString even without
standard-library installation. With Error installed they inherit its registered
prototype. Their ordinary diagnostic graph can be restored from a file by another
process, including the built-in method. See [STANDARD_ERROR.md](STANDARD_ERROR.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ABI_VERSION` | macro / define | [include/flashvm/flashvm.h:25](../include/flashvm/flashvm.h#L25) |
| `fvm_debug_exception_events` | function | [include/flashvm/exception.h:70](../include/flashvm/exception.h#L70) |
| `fvm_debug_resource_errors` | function | [include/flashvm/debug.h:156](../include/flashvm/debug.h#L156) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `fvm_exception_clear` | function | [include/flashvm/exception.h:65](../include/flashvm/exception.h#L65) |
| `fvm_exception_get` | function | [include/flashvm/exception.h:61](../include/flashvm/exception.h#L61) |
<!-- source-links:end -->
