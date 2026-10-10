#ifndef FLASHVM_FLASHVM_H
#define FLASHVM_FLASHVM_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#if defined(_WIN32) && defined(FVM_SHARED_BUILD)
# if defined(FVM_EXPORTS)
#  define FVM_API __declspec(dllexport)
# else
#  define FVM_API __declspec(dllimport)
# endif
#else
# define FVM_API
#endif
#define FVM_ABI_VERSION 1UL
#define FVM_FEATURE_JIT 0x01UL
#define FVM_FEATURE_RENDER 0x02UL
#define FVM_FEATURE_AUDIO 0x04UL
#define FVM_FEATURE_VIDEO 0x08UL
#define FVM_FEATURE_EXTENSIONS 0x10UL
typedef struct FVM_Runtime FVM_Runtime;
typedef struct FVM_Context FVM_Context;
typedef struct FVM_Object FVM_Object;
typedef enum FVM_Result {
    FVM_OK = 0,
    FVM_ERROR = -1,
    FVM_OUT_OF_MEMORY = -2,
    FVM_INVALID_ARGUMENT = -3,
    FVM_UNSUPPORTED = -4
} FVM_Result;
typedef struct FVM_Config {
    unsigned long struct_size;
    unsigned long features;
    unsigned long memory_limit_bytes;
    unsigned long instruction_limit;
    void *userdata;
} FVM_Config;
FVM_API unsigned long fvm_abi_version(void);
FVM_API FVM_Result fvm_config_init(FVM_Config *config);
FVM_API FVM_Result fvm_runtime_create(const FVM_Config *config, FVM_Runtime **out_runtime);
FVM_API void fvm_runtime_destroy(FVM_Runtime *runtime);
FVM_API FVM_Result fvm_context_create(FVM_Runtime *runtime, FVM_Context **out_context);
FVM_API void fvm_context_destroy(FVM_Context *context);
FVM_API FVM_Result fvm_load_memory(FVM_Context *context, const void *data, size_t length);
FVM_API FVM_Result fvm_advance(FVM_Context *context, double milliseconds);
#ifdef __cplusplus
}
#endif
#endif
