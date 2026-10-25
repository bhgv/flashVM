# Versioned module namespace / Версии модулей

[API](include/disvm/module_store.h) · [Implementation](host/module_store.c) ·
[Every-line tutorial](examples/module-store/main.c) · [IDE workspace](../ide/WORKSPACE.md)

`disvm_module_store` is an immutable native-byte namespace shared by compiler,
I/O and loader owners. It is independent of both GC heaps and reuses FlashVM's
actual platform mutex. It neither executes bytecode nor replaces `parsemod`.

## Publication and ownership

Names are absolute logical byte identities, at most 255 bytes, with canonical
POSIX segments. Empty/dot/dot-dot segments, ASCII controls, colon and backslash
are rejected. These are not OS paths, URL parsing, sandbox grants or Styx fids.
Non-ASCII bytes are preserved; this native layer does not normalize Unicode.
The Limbo workspace independently requires valid Unicode and canonical UTF-8.

Publication deep-copies an existing `DVM_ModuleImage`. Expected generation zero
means create-only, the exact current generation means compare-and-swap, and
`DVM_STORE_ANY` means unconditional replacement. Generation IDs never wrap or
repeat within a store. Stale compilation results return CONFLICT without
changing current bytes or the output generation.

A lease pins exactly one immutable version. Replacing/removing a name cannot
invalidate old readers. `dvm_module_lease_view` borrows pointers until that lease
is released; callers must not mutate them or race view use with lease release.
A lease is an exclusive handle, not a freely copied reference-counted pointer.
Close prevents new readers/writers but preserves leases. Destroy returns BUSY
until readers are released; the caller must stop/join all other users first.
Destroy is not concurrent store-handle lifetime management.

The byte quota includes every current and pinned retired image. Replacing an
unleased version credits its old bytes, while pinned readers can make a reload
fail LIMIT. Names, resident versions and reader handles also have independent
hard bounds. Byte accounting excludes allocator overhead, names/metadata and
temporary candidate copies. Concurrent callers retain their own lifetime and
input-buffer responsibilities; this is not an absolute process-memory limit.

## Source adapters and real parsing

A provider may use the existing file, socket, channel, memory or embedded-image
APIs. It runs outside the mutex and may reenter the store. Its resulting name
must match the requested identity. Transport errors remain in `source_status`;
source failure is not converted to a successful publication. Complete/partial
provider images are disposed on every path. The provider must return eventually
and must not destroy the borrowed store. Close does not forcibly cancel I/O.

On the Dis owner, acquire a lease, obtain its view, create an owned image with
`dvm_image_copy`, and call the existing `dvm_image_parse`. Keep normal Dis error,
GC and module ownership boundaries. Release the copy and lease after parsing;
release the resulting Module through Dis unload, never through the byte store.
No module-instance cache, export invocation or cross-heap Module pointer is
introduced by this namespace. Native builtin registration and executable host
closure remain unfinished.

## Validation

The native regression checks copied inputs/listings, stale generations,
reader-pinned replacement/removal, byte/name/version/reader quotas and recovery,
provider reentry/failure cleanup, close/destroy lifetime, and 200 two-OS-thread
CAS races. AddressSanitizer and UBSan also pass. LeakSanitizer's unavailable
`/proc` inspection is explicitly disabled in that test only; no leak-sanitizer
acceptance is claimed. Original shared-host/GC coordination tests still run.

## Русское пояснение

Это общее пространство имён **байтов модулей**, а не готовый исполняемый хост.
IDE может сохранить новую сборку в память, загрузчик — удержать прежнюю версию,
а новый читатель — получить новую. Старые читатели не получают освобождённую
память; устаревшая сборка не затирает свежую. Все удерживаемые версии входят
в квоту. Ввод остаётся в существующих адаптерах FlashVM, владение Dis-модулем
и сборка его мусора остаются на стороне Dis. Sys/VFS/9P и полный запуск IDE
этим API не объявляются реализованными.
