# Directory cancellation / Отмена перечисления каталогов (runtime0.159)

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
  cancellable path operations and its delegated stream advertises LIST (or the
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

Dynamic namespace directory handles deliberately **do not advertise LIST yet**.
A new bind can introduce a legacy, noncancellable provider after open. Therefore
guest `Sys.dirread` still uses its existing admission path: this change does not
claim admitted guest directory cancellation. A future bridge must check each
newly selected provider and its cleanup boundary without freezing the tree or
holding a namespace mutex across I/O. Capture/bind acquisition and guest close
integration also remain separate.

Динамический bind может добавить провайдер без поддержки отмены. Поэтому общий
namespace-каталог пока не обещает LIST, а гостевой `dirread` не переключён на новый
режим. Проверки прекращения обхода уже действуют под явно заданным native scope.

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
The complete native suite now has239 cases; run both normal and ASan/UBSan builds.
