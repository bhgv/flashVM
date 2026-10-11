#ifndef FLASHVM_EXTENSIONS_H
#define FLASHVM_EXTENSIONS_H
#include "value.h"
#include "channel.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Opt-in ActionScript namespaces: gc, thread and channel. Idempotent when
   installed namespaces are intact. Existing globals are not overwritten. */
FVM_API FVM_Result fvm_extensions_install(FVM_Context *);
/* Retained wrapper usable by ActionScript; publish with fvm_set_global.
   Wrapper lifetime is managed by GC, each VM has its own wrapper object. */
FVM_API FVM_Result fvm_extensions_wrap_channel(FVM_Context *, FVM_Channel *, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
