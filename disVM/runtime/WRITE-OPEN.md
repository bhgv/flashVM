# Existing write-only opens / Открытие существующего файла на запись

Copyright (c) 2026 bhgv

`Sys.open(path, Sys.OWRITE)` opens an existing file for writes at offset zero.
It does not create, truncate or append, and does not require read permission.
Unsupported mode combinations, including OTRUNC/ORDWR, still fail explicitly.
`Sys.create` retains its existing OWRITE/0666 contract. No new Sys wire IDs,
namespace credentials or FD groups are introduced.

`Sys.open(path, Sys.OWRITE)` открывает существующий файл на запись с позиции ноль.
Нет создания, усечения, append или требования read permission. OTRUNC/ORDWR и
другие неподдержанные сочетания по-прежнему отвергаются. Sys.create сохраняет
прежний контракт. Это не реализация guest credentials или изоляции NEWNS.

## Ownership and native operations / Владение и native operations

[Sys_open](host_runtime/system_entries.inc) roots the unpublished FD through the
existing native wait. [Resource selection](vfs/resource.c) freezes the namespace
plan and retains its directory. The [file port](../../flashVM/ports/file_port.h)
adds `wo`, distinct from truncating `w`, exclusive `wx` and append-oriented `rw`.
The native-only private `wb-` spelling becomes O_WRONLY without O_CREAT/O_TRUNC;
fdopen wraps that descriptor without reopening it. Failed fdopen closes the FD.

Sys_open удерживает неопубликованный FD на native wait. Resource plan удерживает
выбранный каталог. Режим wo отличается от w/wx/rw; private wb- переводится в
O_WRONLY без O_CREAT/O_TRUNC. fdopen не переоткрывает файл. При его отказе
native FD закрывается. Прежние gate/retirement и GC publication сохраняются.

Kernel credentials enforce actual host rwx/ACL rules. A file with mode 0222 can
be opened without read rights; a bound file with mode 0444 refuses OWRITE.
Existing symlink behavior is unchanged; no sandbox or per-guest identity is claimed.
Write does not confer read permission, and seek changes the write cursor. Sys
and port writes are not transactional; ordinary errors retain existing semantics.

Kernel применяет host rwx/ACL. Проверяются 0222 без read и 0444 через bind.
Symlinks сохраняют прежнее поведение; sandbox/per-guest identity не заявлены.
Запись не даёт права чтения. Seek меняет write cursor; транзакционности записи нет.

## Acceptance / Приёмка

[test_namespace_permissions.py](test_namespace_permissions.py) runs the actual
[Limbo fixture](fixtures/namespace-permissions/main.b) in interpreter/JIT and both
poll paths. Independent host checks require ABCDE to become XBCZE, preserving
length and untouched bytes; no missing file may appear. It checks read refusal
through the write-only FD, writable-directory refusal, rejected OTRUNC, bound
readonly refusal and directory-search rights. Non-root host identity is required.

[test_anchor_port.c](vfs/test_anchor_port.c) covers both ordinary and directory-
relative wo, missing paths and late fdopen failure with descriptor closure and
unchanged bytes. Full native wide remains 70 workloads, with the extended
permission fixture in the existing namespace-permissions workload. Target
architecture and TSan acceptance are established only by completed CI jobs.

Linux AMD64 is checked locally. POSIX adapters share implementation; physical
macOS/Android acceptance is pending. Windows CRT native-path code is implemented
but untested; Windows directory anchors remain explicitly unsupported.
