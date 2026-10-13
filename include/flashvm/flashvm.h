/* FlashVM: Public embedding API declarations: flashvm.
 * File: include/flashvm/flashvm.h.
 */

#ifndef FLASHVM_FLASHVM_H
/* Define FLASHVM FLASHVM H constant or compilation switch. */
#define FLASHVM_FLASHVM_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
#if defined(_WIN32) && defined(FVM_SHARED_BUILD)
#if defined(FVM_EXPORTS)
/* Define declspec macro. */
#define FVM_API __declspec(dllexport)
#else
/* Define declspec macro. */
#define FVM_API __declspec(dllimport)
#endif
#else
#define FVM_API
#endif
/* Define FVM ABI VERSION constant or compilation switch. */
#define FVM_ABI_VERSION 3UL
/* Define FVM FEATURE JIT constant or compilation switch. */
#define FVM_FEATURE_JIT 0x01UL
/* Define FVM FEATURE RENDER constant or compilation switch. */
#define FVM_FEATURE_RENDER 0x02UL
/* Define FVM FEATURE AUDIO constant or compilation switch. */
#define FVM_FEATURE_AUDIO 0x04UL
/* Define FVM FEATURE VIDEO constant or compilation switch. */
#define FVM_FEATURE_VIDEO 0x08UL
/* Define FVM FEATURE EXTENSIONS constant or compilation switch. */
#define FVM_FEATURE_EXTENSIONS 0x10UL
    typedef struct FVM_Runtime FVM_Runtime;
    typedef struct FVM_Context FVM_Context;
    typedef struct FVM_Object FVM_Object;
    /* Define FVM_Result enum and its representation. */
    typedef enum FVM_Result
    {
        FVM_OK = 0 /* Operation completed successfully. */,
        FVM_ERROR = -1 /* Operation failed without a more specific result code. */,
        FVM_OUT_OF_MEMORY = -2 /* Required allocation could not be completed. */,
        FVM_INVALID_ARGUMENT = -3 /* Input violates the API argument contract. */,
        FVM_UNSUPPORTED = -4 /* Requested feature is not supported by this configuration. */,
        FVM_NOT_FOUND = -5 /* Requested entry or nonblocking result is unavailable. */,
        FVM_EXCEPTION = -6 /* An ActionScript exception is pending. */
    } FVM_Result;
    /* Define FVM_Config struct and its representation. */
    typedef struct FVM_Config
    {
        unsigned long struct_size;        /* Size of this structure for API compatibility checks. */
        unsigned long features;           /* Requested runtime feature bits. */
        unsigned long memory_limit_bytes; /* Configured memory budget in bytes. */
        unsigned long instruction_limit;  /* Maximum charged work: AVM1 instructions and native Array slice/join/concat traversal. */
        void *userdata;                   /* Opaque caller data passed to native callbacks. */
    } FVM_Config;
    /* Implement abi version for this subsystem. */
    FVM_API unsigned long fvm_abi_version(void);
    /* Initialize config. */
    FVM_API FVM_Result fvm_config_init(FVM_Config *config);
    /* Create runtime. */
    FVM_API FVM_Result fvm_runtime_create(const FVM_Config *config, FVM_Runtime **out_runtime);
    /* Destroy runtime. */
    FVM_API void fvm_runtime_destroy(FVM_Runtime *runtime);
    /* Create context. */
    FVM_API FVM_Result fvm_context_create(FVM_Runtime *runtime, FVM_Context **out_context);
    /* Destroy context. */
    FVM_API void fvm_context_destroy(FVM_Context *context);
    /* Load memory. */
    FVM_API FVM_Result fvm_load_memory(FVM_Context *context, const void *data, size_t length);
    /* Implement advance for this subsystem. */
    FVM_API FVM_Result fvm_advance(FVM_Context *context, double milliseconds);
#ifdef __cplusplus
}
#endif
#endif
