# Directory cancellation / Отмена перечисления каталогов (runtime0.159–0.160)

The native Styx mount adapter now consumes the caller's `DVM_CancelScope` during
`dvm_vfs_list`. It uses the original 9P `Tread`, directory stat records and
`Tflush`; no tree serialization or wire extension is introduced. Opened directory
handles from fd-owner clients advertise `DVM_VFS_CANCEL_LIST` (8). Legacy client
transports do not advertise it. `DVM_VFS_CANCEL_MASK` is now15; rebuild SDK consumers.
Callback tables and the guest Sys ABI are unchanged.

Native embedding code opens/pins the handle, enters a scope with its own native
cancellation source, calls `dvm_vfs_list`, leaves the scope and eventually frees
returned entries with `fvm_port_directory_entries_free`. Another native thread
may request that source. Serialize the cursor through the entire operation; the
signal callback retains only a native cancellation token, never a provider,
namespace, guest or cursor pointer. See [source lifetime/API](DATA-CANCEL.md).

## Acknowledgements and cursor lifetime / Подтверждения и курсор

- Cancellation before a raw directory read submits no `Tread`. No records were
  consumed, so that raw cursor can be retried under a fresh scope.
- An acknowledged `Rread` wins a reply/flush race. The raw adapter publishes its
  complete records and advances only by acknowledged bytes. A genuine acknowledged
  empty `Rread` is EOF. Complete buffered records require no new RPC.
- A failed, uncertain or malformed read invalidates that directory cursor. It
  returns failure with no entries, and subsequent calls fail instead of reporting
  EOF or skipping records. Reopen to start an independent enumeration.
- A prepared view additionally checks each child's current visibility, metadata
  and rights. It advertises LIST only when every explicit source promises
  cancellable path and directory-chain operations and its delegated stream advertises LIST (or the
  directory is purely synthetic). Mutable source contents are still live.
- Once the source is cancelled, the view starts no new child metadata probes and
  no next hidden batch. It returns any already checked complete prefix; with no
  checked entries it returns failure. The interrupted view cursor then fails on
  further reads. Raw acknowledged bytes alone cannot bypass the view's policy.
- A cancelled failed lookup cannot synthesize a virtual parent as a successful
  metadata result. An individual metadata RPC timeout without source cancellation
  retains the existing view rule: an unavailable child's metadata hides that
  child; it does not itself cancel the caller's logical scope.

Обычный Styx-каталог сохраняет подтверждённые полные записи. Неподтверждённое
чтение и повреждённые записи завершаются ошибкой, а не EOF. Подготовленный view
может вернуть только уже проверенный префикс; отмена не разрешает выдать остальные
имена без проверки прав. Для продолжения прерванного view нужно открыть новый
курсор. Содержимое удалённого дерева не фиксируется.

## Dynamic namespace boundary / Граница динамического namespace

Union enumeration checks the current native scope before entering a branch and
after an empty/failed result. Cancellation cannot become a successful empty
batch or open the next provider. Namespace child projection also stops before
another child query or fully shadowed batch. These guards apply whenever an
embedding caller supplies a scope; they do not require guest cooperation.

Runtime0.160 adds [guest directory waits](GUEST-DIRECTORY-WAITS.md). Eligible
namespace/union handles now advertise LIST after conservative open-time checks.
Every newly selected route is guarded again. A late legacy bind fails before
its callback and requires reopening, instead of breaking the cancellation promise.
The tree remains live and normal legacy enumeration remains available after reopen.

Runtime0.160 подключает гостевой dirread для подходящих namespace/union handles.
Поздний legacy bind вызывает отказ до callback; требуется переоткрытие каталога.
Дерево остаётся живым, прежнее перечисление legacy-каталогов сохраняется.

## Verification / Проверки

Twenty-four real original-codec TCP cases cover raw/prepared directory reads,
first/second metadata probes, pre-cancel, reply/flush races, lost flush and timeout.
Two more inject a truncated directory stat record into an otherwise original
`Rread`. The driver checks actual request counts, no replay, no extra metadata
probes, preserved checked prefixes, cursor retirement and real EOF.

A native namespace/union test covers failure and apparent EOF after a branch
requests cancellation. Independent counters require zero lower-provider opens
and reads. A private relink removing the union guards makes that test fail on
lower-provider entry. Production objects stay unchanged by the negative control.
The complete native suite now has248 cases (including scoped directory-close coverage); run both normal and ASan/UBSan builds.
