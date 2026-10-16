/* FlashVM: counted intrinsic names read without callbacks for watcher old values. */
#ifndef FVM_CORE_PROPERTY_WATCH_CONSTANTS_H
#define FVM_CORE_PROPERTY_WATCH_CONSTANTS_H /* Guard for the private watcher-name constants. */
#define FVM_WATCH_ARRAY_LENGTH_NAME "length" /* Native Array resource-length field. */
#define FVM_WATCH_ARRAY_LENGTH_UNITS (sizeof(FVM_WATCH_ARRAY_LENGTH_NAME) - 1U) /* UTF-16 units. */
#define FVM_WATCH_CALLBACK_SUPER_DEPTH 0U /* Loaded watcher super starts at the receiver. */
#endif
