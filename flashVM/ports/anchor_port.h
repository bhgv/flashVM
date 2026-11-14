/* Copyright (c) 2026 bhgv
 * EN: Owned directory identity and relative operations; no OS types in this API.
 * RU: Владение identity каталога и relative operations; без типов ОС в API.
 */
#ifndef FVM_ANCHOR_PORT_H
#define FVM_ANCHOR_PORT_H
#include "filesystem_port.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct FVM_PortAnchor FVM_PortAnchor; /* Immutable OS directory handle, externally retained. */


/* EN: Open path as a stable directory anchor. Return owned *out on success;
 * error leaves *out NULL. Blocking, no VM access. Close after all users join.
 * RU: Открыть path как стабильный anchor каталога. Успех передаёт *out; при
 * отказе *out=NULL. Блокирует, без VM access. Закрывать после всех users. */
FVM_Result fvm_port_anchor_open(const char *path, FVM_PortAnchor **out);


/* EN: Open a directory relative to borrowed live parent, with an independent
 * OS handle. No path reopening of the parent; relative must not be absolute.
 * RU: Открыть каталог относительно живого parent с независимым OS handle.
 * Parent не переоткрывается по имени; relative не должен быть absolute. */
FVM_Result fvm_port_anchor_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortAnchor **out);


/* EN/RU: Close owned anchor after all concurrent operations finish; NULL allowed.
 * Закрыть anchor после завершения всех операций; NULL допустим. */
void fvm_port_anchor_close(FVM_PortAnchor *anchor);


/* EN: Stat relative path against the held object; no cursor changes. Borrow
 * parent/relative/output, return port status; output changes only on success.
 * RU: Stat relative path от удерживаемого объекта, без изменения cursor.
 * parent/relative/output заимствованы; port status, output меняется при успехе. */
FVM_Result fvm_port_anchor_info_at(FVM_PortAnchor *parent, const char *relative, FVM_FileInfo *output);


/* EN: Open file relative to live parent with ordinary r/w/wx/rw semantics.
 * Transfer independent file to *out on success. Allocate before destructive
 * effects; external synchronization governs each returned file, not parent.
 * RU: Открыть файл от живого parent с обычными r/w/wx/rw. Успех передаёт
 * независимый *out. Allocation до destructive effects; синхронизировать каждый
 * возвращённый файл, не parent. Symlinks/'..' follow native rules, not a sandbox. */
FVM_Result fvm_port_file_open_at(FVM_PortAnchor *parent, const char *relative, const char *mode, FVM_PortFile **out);


/* EN: Open independent directory cursor relative to held parent. Borrow inputs,
 * transfer *out on success; later rename does not redirect enumeration/metadata.
 * RU: Открыть независимый directory cursor от parent. Inputs заимствованы,
 * успех передаёт *out; rename не перенаправляет enumeration/metadata. */
FVM_Result fvm_port_directory_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortDirectory **out);
#ifdef __cplusplus
}
#endif
#endif
