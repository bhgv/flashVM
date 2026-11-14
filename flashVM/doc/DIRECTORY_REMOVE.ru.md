# Удаление entry через удержанный каталог

`fvm_port_anchor_remove_at(parent, relative)` из [anchor_port.h](../ports/anchor_port.h)
удаляет файл, final symlink либо пустой каталог от живого borrowed anchor.
Parent должен жить до конца blocking call, как для relative open/stat. Владение
parent/cursor не передаётся. Независимые операции используют один immutable
parent параллельно; закрывать его можно после завершения borrowers.

Успех — [FVM_OK](../include/flashvm/flashvm.h#L44), неверные parent/path — [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47), OS failure — [FVM_ERROR](../include/flashvm/flashvm.h#L45).
Отсутствующий Windows relative HANDLE backend возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48), без
эмуляции pathname. POSIX операция не добавляет VM callbacks, установленных
helpers, внешних библиотек, recursion или allocation.

Relative path непустой, не absolute, с существующими UTF-8 bounds. Final dot,
dot-dot и пустой final component (включая trailing slash) запрещены. Intermediate
symlinks и native dot-dot сохраняют host semantics; sandbox/containment нет.
POSIX fstatat с AT_SYMLINK_NOFOLLOW выбирает unlink либо empty-directory removal,
затем вызывает unlinkat. Final symlink, в том числе dangling или на каталог,
удаляется сам; target не удаляется. Непустой каталог отклоняется без удаления children.

Child может измениться между classification и removal. Это удаление по parent/
name, не compare-and-delete по inode. Same-kind replacement может быть удалён;
смена kind может дать отказ. Нет retry через другое pathname или rollback после
успеха. Для неизменного общего имени ровно один конкурентный remove успешен,
остальные дают отказ. Durability/fsync transaction и recursive deletion нет.

POSIX открытый файл остаётся доступным после unlink. DisVM удерживает один
namespace selection через native release/reentry; при занятой result cell
повторяет только guarded scalar publication, не удаление.
[Контракт DisVM](../../disVM/runtime/NAMESPACE-REMOVE.ru.md).

Linux x64 real-port: rename удержанного parent с decoys, files, empty/full dirs,
final file/directory symlinks, восемь OS owners за одно удаление, invalid paths,
чтение открытого FD после unlink. Сохранены проверки независимых cursors,
create и позднего stdio failure. Interpreter/JIT пример держит Module code после
удаления image. Физическая приёмка macOS/Android и Windows backend остаются
работой. Стадии Flash не закрываются.

[Anchors](DIRECTORY_ANCHORS.ru.md), [English](DIRECTORY_REMOVE.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_port_anchor_remove_at` | function | [ports/anchor_port.h:63](../ports/anchor_port.h#L63) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
