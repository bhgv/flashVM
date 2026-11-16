# Cancellable open and metadata / Отменяемые open и metadata (runtime0.158)

Guest `Sys.open`, `Sys.stat`, `Sys.fstat` and `Sys.seek` now retain native operation
cancellation while an explicitly capable provider performs their I/O. Existing
direct-child `Processctl.cancelwait/killwait` reaches the native scope and Styx
Tflush. The source guest does not execute a callback, poll for cancellation,
change its Prog/REG owner or pause for inspection.

Область отмены расширена с read/write на open/stat, fstat и seek. Для path/open
она устанавливается до native release и сохраняется на всю цепочку служебных
RPC, включая освобождение временных fid. Для FD сохраняется его обычная FIFO:
отмена активного владельца не снимает его ticket до завершения операции.

## Native capabilities / Native capabilities

| API/flag | Contract / Контракт |
|---|---|
| `dvm_provider_create_native(ops,state,capture,fixed_root,native_flags)` | Existing creation/ownership contract plus an immutable native capability mask. State is adopted only on success. |
| `DVM_PROVIDER_CANCEL_PATH` | Factory info/open/access honors the current cancellation scope, including temporary-resource cleanup. Does not promise cancellable capture or grant rwx. |
| `dvm_provider_native_flags(provider)` | Immutable mask/0, borrowed retained factory, no I/O. |
| `DVM_VFS_CANCEL_INFO` | Open handle metadata honors native cancellation. |
| `DVM_VFS_CANCEL_SEEK` | Seek, including its metadata RPC, honors native cancellation. |
| `dvm_vfs_path_enter` | Host-only ACTIVE-source registration/release without an FD ticket; source owns the retained path selection. |

Old `dvm_provider_create` and `create_capture` constructors keep a zero native
capability mask. Their callback tables and behavior are retained. The
`DVM_ProviderOps`/`DVM_VfsOps` layouts, guest Sys ABI and Styx messages are unchanged.
The `native_flags` handle field was introduced in runtime0.157; these flags use
that existing field rather than extending its layout again.

Prepared views declare path capability only when every explicit source supports
it. Hiding rules do not add a source. Path adapters require **every retained
candidate** to support control cancellation; mixed host/legacy stacks retain
legacy admission. A flag is a trusted native implementation promise, not a
permission, liveness guarantee or request to infer access from displayed mode.

Обёртки провайдеров и view сохраняют INFO/SEEK capabilities. Fstat обычного
projected/union-каталога относится к его удержанному header, поэтому INFO может
передаваться и через эти обёртки. Это не означает отменяемую enumeration:
перечисление каталога требует отдельных гарантий для динамических нижних слоёв
и подставленных namespace-узлов.

## Styx control chain / Цепочка служебных RPC

The native mount adapter binds a fresh single-use token to each logical walk,
stat, open and temporary-fid clunk under the current source scope. A fragmented
walk still has one token across its MAXWELEM fragments. Source cancellation is
sticky, so the next step cannot escape a request delivered between RPCs.
Callbacks retain only native tokens; native workers retain client/fid-number
capabilities, never provider, Prog or guest pointers.

The adapter uses the original client/codecs and [existing Tflush rules](../client/MULTIPLEX.md).
Tstat addresses the held fid. A sent unacknowledged Topen is not retried; its
native fid is fenced, then retired. An acknowledged Ropen remains an opened
resource and is returned to the caller even when cancellation was requested.
Guest termination still releases it through ordinary ownership cleanup.

`styxclientclunkcancel(fid, cancel)` consumes its fid on every outcome. The
fd-owner Tclunk is cancellable; a legacy client uses independent native cleanup.
An unacknowledged request schedules one best-effort cleanup of that same
monotonic, never-reused fid number. A known Rerror is not retried. This cleanup
does not replay an open, write or device-data operation. Repeating Tclunk for an
already retired, never-reused number cannot close a later client fid.

`styxclientclunklater(fid)` consumes the fid without a caller RPC wait; an
independent native worker owns its client/number. This is also the cleanup path
when cancellation binding itself cannot be allocated. Existing bounded/quota
and best-effort teardown semantics apply; setup failure fails the connection
without waiting on a guest. No reconnect or fid-number reuse is added.

## Acknowledgements and union selection / Подтверждения и union

Complete acknowledged metadata is retained even when cancellation interrupts its
subsequent Tclunk. Ordinary union metadata returns the selected acknowledged record
without immediately issuing a duplicate lookup; a later stat still queries live state. Failed or unacknowledged metadata leaves output untouched.
Failed SEEK_END leaves both its output cell and actual cursor unchanged.
Acknowledged Ropen/seek results win over cancellation intent, including the
existing case where an original reply is known but a required Rflush is lost;
the latter still terminates the connection.

Cancellation is not "not found". After an interrupted upper metadata lookup,
ordinary union selection and MAUTORWX stop before querying another provider.
They do not reinterpret cancellation as permission to open, create or inspect a
lower resource. This guard changes no ordinary uncancelled union ordering or
creation policy. Native flags do not replace the operation's access decision.

Отмена не превращается в отсутствие файла и переход к следующему провайдеру.
Подтверждённые metadata/open не теряются, неизвестные эффекты не повторяются.
Если отмена пришла во время cleanup после успешного Tstat, запись атрибутов
остаётся подтверждённой; Tclunk имеет отдельное владение и результат.

## Use / Использование

Guest code keeps using the existing API:

```limbo
# The native host installed an authorized opt-in provider at /remote.
# Owner receives a direct child's PID using its ordinary application channel.
status := control->cancelwait(childpid);
# Alternatively killwait requests termination and waits for guest retirement.
retired := control->killwait(childpid);
```

The [complete interpreter/JIT guest](../../runtime/fixtures/vfs-control-wait/main.b)
demonstrates prepared views, admitted path/open/fstat/seek waits, directory-fid
metadata, acknowledged open and stopped union fallback. Native embedding code
uses the same [operation scopes](DATA-CANCEL.md); no Limbo interception loop is
required to deliver cancellation.

## Verification and remaining scope / Проверки и оставшаяся интеграция

The native suite has212 tests. Thirty added real original-codec TCP cases cover
path metadata/open, held-fid stat, SEEK_END and cleanup, each with success,
pre-cancel, cancellation, reply/Rflush race, lost Rflush and timeout. They verify
prepared-view capabilities, acknowledged results, untouched failed outputs,
cursor preservation, cleanup and continued unrelated connection usability.

The dedicated native Dis guest runs15 actual admitted control jobs in interpreter
and JIT. It checks cancelwait/killwait, normal and directory fstat, post-cancel
fresh operations, retained acknowledged FD/metadata ownership, ordinary/MAUTORWX union
stop behavior, cancelled create-candidate refusal, cleared scopes before managed reentry and exact job/FD drain.
A private relink disabling the automatic-selection cancellation guards probes
a lower provider; the independent fixture counter rejects it. Production objects
are unchanged by this negative control. The native suites also run with ASan/UBSan (`detect_leaks=0`) on Linux amd64.

[Native directory enumeration](DIRECTORY-CANCEL.md) now consumes explicit scopes;
[Guest directory waits](GUEST-DIRECTORY-WAITS.md) now cover eligible live namespace cursors.
[Capture/bind acquisition](ACQUIRE-CANCEL.md) now registers eligible operations.
[Detached close shutdown](CLOSE-CANCEL.md) now supports eligible native cleanup;
Sys reference release remains asynchronous. Enumeration must preserve complete stat records and not
convert a cancelled branch to EOF or continue into another dynamically selected
provider. A guest Sys.mount transport builtin and network authentication are
not introduced by these cancellation adapters. Other platforms are not claimed.
