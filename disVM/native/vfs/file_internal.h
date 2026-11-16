/* Copyright (c) 2026 bhgv
 * EN: Private native node/open ownership shared by file and directory adapters.
 * RU: Private-владение native node/open для файлового и каталогового адаптеров.
 */
#ifndef STYX_FILE_INTERNAL_H
#define STYX_FILE_INTERNAL_H
#include "file_provider.h"
#include "resource.h"
struct StyxBacking {
	DVM_Provider *provider; /* Retained prepared namespace root. */
	char path[DVM_NS_PATH_LIMIT]; /* Canonical relative path, dot is root. */
	StyxMeta captured; /* Bound identity for tree nodes; no contents snapshot. */
	int bound; /* File-only legacy wrapper keeps live root lookup behavior. */
};
struct StyxOpen {
	DVM_VfsHandle handle; /* Exclusive opened object. */
	unsigned long long position; /* Confirmed byte position. */
	int poisoned; /* Unknown data effects forbid further data I/O. */
	unsigned access; /* Open-mode ceiling. */
	int directory; /* Directory list APIs, never raw read/write. */
	DVM_Provider *provider; /* Directory rewind/refresh capability. */
	char path[DVM_NS_PATH_LIMIT]; /* Confined node-relative path. */
	StyxMeta captured; /* Opened directory identity; reopens must match. */
	FVM_FileInfo *batch; /* Owned pending provider batch, not a tree snapshot. */
	size_t count, index; /* Pending batch bounds. */
	int pending, started, failed; /* Peek/commit and cursor state. */
};
/* EN/RU: Checked metadata copy and object identity comparison, without authority inference.
 * Проверенное копирование metadata и сравнение identity, без вывода полномочий. */
int styxmeta(const FVM_FileInfo *info, StyxMeta *output);
int styxsame(const StyxMeta *left, const StyxMeta *right);
#endif
