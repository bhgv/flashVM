# Directory/node capabilities / Capabilities каталогов и node

Copyright (c) 2026 bhgv

`styxtree(provider)` retains a prepared directory provider. Construction queries
only its root metadata; it does not walk, serialize or freeze a tree. `tree.h`
extends the same scalar C boundary as `file.h`, while `file_provider.h` remains
on the VFS side. No extra 9P message types or second codec are introduced.

`styxtree` удерживает уже разрешённый каталог-provider. Node — это capability
с нормализованным относительным путём и заявленной identity, а не копия дерева.
Право доступа не выводится из mode. Provider должен проверять текущую политику
в своих операциях; guest callbacks и остановки рабочих потоков здесь отсутствуют.

## Nodes and access / Ноды и доступ

- `styxwalknode(parent, component)` returns an owned child/parent capability or
  NULL. One component is accepted; separators, empty names and oversized names
  are rejected before provider callbacks. `.` stays at the node; root `..` stays
  at the retained provider root. No operation resolves a wire name as a host path.
- Walk requires the current node to remain a directory with its bound identity,
  and `dvm_provider_access(..., EXEC)` to succeed. Missing/unsupported effective
  access checks fail closed; mode is not a substitute for effective authority.
- Node metadata and opens reject observed identity/device/type changes. Metadata
  is live, not copied permanently from the first walk. These separate callbacks
  do not promise an atomic namespace transaction: providers remain responsible
  for confinement, reliable identities and correct open/metadata semantics.
- `styxfileopen(node, 0)` opens either a file or a directory. Directories are
  read-only and use cursor APIs; raw byte read/write refuses directory handles.
  File constructors and modes from the earlier bridge remain available.

На каждый открытый fid передаётся собственный handle через `styxsetaux`; cleanup
выполняет `freefid`. Пример таблицы посещённых Qid/node находится только в тесте:
это не готовый общий сервер и не API реестра namespace. Привязка prepared root
к проверенному удалённому клиенту/его attach остаётся отдельным этапом.

## Directory cursors / Курсоры каталогов

1. Call `styxdirbegin(opened, offset)` once per read request. Nonzero offsets must
   equal the sum of committed wire record lengths. Zero restarts enumeration.
2. `styxdirpeek` returns 1 for a current child, 0 for actual provider EOF and -1
   for failure. It retains only the current provider batch. Every pending child
   refreshes its current metadata/visibility before publication. A disappeared
   or hidden NOT_FOUND child is skipped; other metadata failures are errors.
3. Encode the complete stat record with the original codec. Only after success
   call `styxdiradvance(opened, actual_record_bytes)`. A too-small positive buffer
   must return an error without committing the entry, never a false EOF.
4. If some records were already emitted before the next record fails, a caller
   may return that confirmed prefix. The cursor remains failed for continuation;
   an explicit restart at zero can recover after policy/provider recovery.

The caller keeps its native owner/FIFO; no implicit dispatch, guest safepoint or
cooperative scheduler is involved. Cursor methods must not race close or another
operation on the same handle. Directory access rechecks effective READ|EXEC and
current directory identity; stale cached mode bits cannot grant listing access.

Rewind opens a fresh cursor only through the retained provider and relative path.
It compares the newly opened identity/device/type against the held directory
before replacing its cursor. Failure preserves the old open and refuses the
request; it does not silently attach to a different folder. Provider identity
quality and virtual-directory semantics are the backend's contract. Ordinary
open view handles now query captured-object metadata with current masking;
synthetic/composed virtual parents retain their view-defined metadata semantics.

Offset=0 не фиксирует каталог: следующий запрос видит текущее состояние. Изменения
между порциями могут дать обычные пропуски/повторы изменяемого каталога. Имя из
provider batch проверяется как один компонент; malformed names инвалидируют
курсор вместо обхода namespace. Для восстановления требуется явный restart.
Существующие открытия удерживают provider после снятия publication/node caps.

## Covered behavior / Что проверено

The real original-libstyx TCP fixture consumes a genuine local directory
publication grant and covers:

- hidden nodes absent from listing and refused by walk;
- one complete record per small page, invalid continuation refusal without
  losing the pending record, and no fake EOF on a too-small buffer;
- nested directories/files and root-clamped parent traversal;
- policy changes after a batch was fetched: pending hidden nodes do not leak;
- revocation/restoration for held files and directories; refusal does not become
  success merely because reported mode still looks readable;
- new children and mode changes visible through the same open directory fid;
- retained file/directory opens after visited node/publication capabilities drop;
- replaced directories refused on restart, while old file opens remain usable;
- every actual provider open closed, final factory destruction and zero escaping
  relative paths reaching the backing provider.

Native cases also test malformed provider names and recovery, refusing path
rebinding, captured view metadata after replacement and opaque byte endpoints
without handle-stat. These tests do not establish full walk/partial-walk semantics,
remote authentication, create/remove/wstat, mount RPC or production admission.
The fixture control endpoint is synthetic test machinery, not a service API.
Графика/FreeType и код чужого scheduler не импортируются.
