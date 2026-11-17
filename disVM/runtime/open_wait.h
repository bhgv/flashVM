/* Private unpublished native handle construction; no managed values cross it. */
#ifndef DVM_OPEN_WAIT_H
#define DVM_OPEN_WAIT_H
#include "vfs/resource.h"
#include "file_port.h"
#include "filesystem_port.h"


/* EN/RU: Reserve/publish one native descriptor for borrowed path/mode/selection; FD/-1.
 * ACTIVE owner keeps selection pinned through native admission; no namespace re-resolution.
 * Занять/выдать descriptor для borrowed selection; FD/-1, ACTIVE owner удерживает source. */
int dvm_runtime_open_selected(const char *path, const char *mode, int asynchronous, DVM_ResourcePlan *selected);


/* Snapshot borrowed UTF-8 path before optionally releasing the active heap.
 * mode is an immutable native r/w/wx literal. file/directory are native caller
 * cells receiving one owned handle or NULL. No descriptor-table access occurs
 * outside the heap. Return a port status; caller publishes or closes the handle.
 * Allocation failure performs no filesystem operation. No managed allocation. */
FVM_Result
dvm_open_wait(const char *path, const char *mode, int asynchronous, FVM_PortFile **file, FVM_PortDirectory **directory);


/* EN/RU: Use borrowed already retained selection without re-resolving namespace.
 * Использовать уже удержанный selection без повторного namespace lookup.
 * Other arguments/ownership follow dvm_open_wait / Остальные аргументы как выше. */
FVM_Result dvm_open_wait_selected(const char *path,
                                  const char *mode,
                                  int asynchronous,
                                  DVM_ResourcePlan *selected,
                                  FVM_PortFile **file,
                                  FVM_PortDirectory **directory);


/* Emit aggregate open attempts/releases and staging after all owners join.
 * No parameters/return, allocation or managed access; requires heap exclusion. */
void dvm_open_statistics(void);

/* EN/RU: Capture native/provider selection once; return native handles or one
 * provider_output (initially empty). Provider opens use native admission only.
 * Захватить selection один раз; вернуть native handles либо provider_output
 * (изначально пустой). Provider open выполняется только в native admission. */
FVM_Result dvm_open_wait_resource(const char *path, const char *mode, int asynchronous,
                                  DVM_ResourcePlan *selected, FVM_PortFile **file,
                                  FVM_PortDirectory **directory, DVM_VfsHandle *provider_output);
#endif
