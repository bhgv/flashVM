/* FlashVM: Public embedding API declarations: extensions.
 * File: include/flashvm/extensions.h.
 */

#ifndef FLASHVM_EXTENSIONS_H
/* Define FLASHVM EXTENSIONS H constant or compilation switch. */
#define FLASHVM_EXTENSIONS_H
#include "value.h"
#include "channel.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Opt-in ActionScript namespaces: gc, thread, channel, metatable and mutex. Idempotent when
       installed namespaces are intact. Existing globals are not overwritten. */
    /* Implement extensions install for this subsystem. */
    FVM_API FVM_Result fvm_extensions_install(FVM_Context *);
    /* Retained wrapper usable by ActionScript; publish with fvm_set_global.
       Wrapper lifetime is managed by GC, each VM has its own wrapper object. */
    /* Implement extensions wrap channel for this subsystem. */
    FVM_API FVM_Result fvm_extensions_wrap_channel(FVM_Context *, FVM_Channel *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
