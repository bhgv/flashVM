/* FlashVM: stable Object/Array callback identities for graph transport. */
#ifndef FVM_STANDARD_BASE_CONSTANTS_H
#define FVM_STANDARD_BASE_CONSTANTS_H
/* Array length getter; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_ARRAY_LENGTH 600UL
/* Array length setter; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_ARRAY_RESIZE 601UL
/* Object prototype getter; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_PROTOTYPE_GET 602UL
/* Object prototype setter; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_PROTOTYPE_SET 603UL
/* ASSetPropFlags callback; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_SET_FLAGS 604UL
/* Object constructor; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_OBJECT_CONSTRUCTOR 605UL
/* Array constructor; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_ARRAY_CONSTRUCTOR 606UL
/* Object function invocation; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_OBJECT_CALL 607UL
/* Script property getter bridge; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_SCRIPT_PROPERTY_GET 608UL
/* Script property setter bridge; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_SCRIPT_PROPERTY_SET 609UL
/* First callback in the existing Object method table; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_OBJECT_METHOD_BASE 610UL
/* Legacy locale alias decoded as ordinary object string conversion; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_LEGACY_OBJECT_STRING 612UL
/* Object.registerClass callback; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_REGISTER_CLASS 626UL
/* Object locale conversion; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_OBJECT_LOCALE 627UL
/* Array function invocation distinct from construction; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_ARRAY_CALL 628UL
/* First callback in the existing Array method table; preserve the historical wire identity. */
#define FVM_BASE_CALLBACK_ARRAY_METHOD_BASE 630UL
/* Global stopAllSounds; keep this outside the historical ordered method ranges. */
#define FVM_BASE_CALLBACK_STOP_ALL_SOUNDS 690UL
#endif
