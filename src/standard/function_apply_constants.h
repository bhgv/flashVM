/* FlashVM: apply length representation and retained host/loaded argument bounds. */
#ifndef FVM_STANDARD_FUNCTION_APPLY_CONSTANTS_H
#define FVM_STANDARD_FUNCTION_APPLY_CONSTANTS_H /* Private apply constants guard. */
#define FVM_FUNCTION_APPLY_LENGTH_NAME "length" /* Own raw argument-list length key. */
#define FVM_FUNCTION_APPLY_LENGTH_UNITS (sizeof(FVM_FUNCTION_APPLY_LENGTH_NAME) - 1U) /* ASCII UTF-16 units. */
#define FVM_FUNCTION_APPLY_HOST_LIMIT 65536U /* Existing unloaded apply argument limit. */
#define FVM_FUNCTION_APPLY_LOADED_LIMIT 65536U /* Retained loaded bound pending incremental budgeted capture. */
#define FVM_FUNCTION_APPLY_INDEX_KEY_BYTES 11U /* Ten decimal Uint32 digits plus terminator. */
#endif
