/* Copyright (c) 2026 bhgv
 * EN: Bind a live FlashVM object to a loadable libstyxrpc module image.
 * RU: Привязка живого объекта FlashVM к загружаемому образу модуля libstyxrpc.
 */
#ifndef FLASHVM_STYXRPC_MODULE_H
#define FLASHVM_STYXRPC_MODULE_H
#include "flashvm/value.h"
#include "styxrpc/module_client.h"
#include "styxrpc/channel_arguments.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FVM_StyxRpcObjectImage FVM_StyxRpcObjectImage; /* Owned publication pin and image. */

typedef struct FVM_StyxRpcObjectMethod
{
    const char *name;        /* Public function name in the compiled module. */
    const char *method;      /* AS member resolved on the live object. */
    StyxRpcModuleKind kind; /* Codec/ordinary callback profile. */
    uint32_t signature;      /* Compiled signature, STYXRPC_INT_SIGNATURE for scalar int. */
    StyxRpcSchema schema;   /* Exact compiled arguments and result, ignored for scalar int. */
    unsigned slots;         /* Channel factory FIFO slots, zero for rendezvous. */
} FVM_StyxRpcObjectMethod;

typedef struct FVM_StyxRpcObjectVariable
{
    const char *name;     /* Public name in .var. */
    const char *property; /* Existing live script property of exact signed int type. */
    int writable;         /* 0 read-only, 1 read/write. */
} FVM_StyxRpcObjectVariable;

typedef struct FVM_StyxRpcObjectSpec
{
    const char *name;                           /* Basename of the image. */
    FVM_Object *object;                          /* Receiver to retain in the owner context. */
    const FVM_StyxRpcObjectMethod *methods;     /* One explicit binding per exported entry. */
    size_t method_count;                        /* Number of exported methods. */
    const FVM_StyxRpcObjectVariable *variables; /* Optional properties in .var. */
    size_t variable_count;                      /* Number of published properties. */
    StyxRpcModuleChannels *registry;            /* Borrowed private registry for live channels. */
    StyxRpcControl *control;                    /* Optional borrowed playback controls at server root. */
} FVM_StyxRpcObjectSpec;


/* [GC:] EN: Create a retained object image on the context owner; copy descriptors and names.
 * RU: Создать образ у владельца контекста с удержанием объекта и копиями имён и описателей. */
FVM_Result fvm_styxrpc_object_image_create(FVM_Context *context, const FVM_StyxRpcObjectSpec *spec,
    FVM_StyxRpcObjectImage **out);


/* EN: Borrow the module backing for styxrpc_module_tree through image destruction.
 * RU: Заимствовать образ для styxrpc_module_tree до уничтожения. */
StyxRpcModule *fvm_styxrpc_object_image_module(FVM_StyxRpcObjectImage *image);


/* [GC:] EN: Destroy after all using servers, on the owner and before the context.
 * RU: Освободить после всех серверов у владельца и до уничтожения контекста. */
void fvm_styxrpc_object_image_destroy(FVM_StyxRpcObjectImage *image);

typedef struct FVM_StyxRpcLoadSpec
{
    struct StyxClient *session;          /* Attached session borrowed through all method calls. */
    const char *path;                    /* Relative path of the mounted Dis module. */
    const StyxRpcModuleImport *methods; /* Exact compiled contract of visible methods. */
    size_t count;                       /* One through STYXRPC_MAX_EXPORTS. */
    uint32_t pid;                       /* Host-provided reverse PID for scoped calls. */
    unsigned wait_ms;                    /* Per-call polling budget in milliseconds. */
    StyxRpcArgumentBus *bus;            /* Borrowed reverse publication if lending channel arguments. */
} FVM_StyxRpcLoadSpec;


/* [GC:] EN: Check a mounted Dis interface and return an AS object of callable methods.
 * RU: Проверить смонтированный интерфейс Dis и вернуть объект AS с вызываемыми методами. */
FVM_Result fvm_styxrpc_module_load(FVM_Context *context, const FVM_StyxRpcLoadSpec *spec, FVM_Value *out);

typedef struct FVM_StyxRpcModuleCatalog
{
    const FVM_StyxRpcLoadSpec *modules; /* Borrowed exact interface declarations selected by the host. */
    size_t count;                      /* Number of supported literal module paths. */
} FVM_StyxRpcModuleCatalog;


/* [GC:] EN: Install an AS function of one path string; borrow the immutable catalog through context destruction.
 * RU: Установить функцию AS с одним путём; заимствовать неизменяемый каталог до уничтожения контекста. */
FVM_Result fvm_styxrpc_module_loader_install(FVM_Context *context, const char *global,
    const FVM_StyxRpcModuleCatalog *catalog);

#ifdef __cplusplus
}
#endif
#endif
