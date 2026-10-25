/* FlashVM: retained ClipAction bounds and strict SWF-version event masks. */
#ifndef FVM_SWF_CLIP_ACTION_CONSTANTS_H
#define FVM_SWF_CLIP_ACTION_CONSTANTS_H            /* Private ClipAction format guard. */
#define FVM_SWF_PLACE_HAS_CLIP_ACTIONS 0x80U       /* PlaceObject2 optional ClipActions field. */
#define FVM_SWF_PLACE_HAS_CLIP_DEPTH 0x40U         /* Unsupported clipDepth field, distinct from events. */
#define FVM_SWF_PLACE_SUPPORTED_FLAGS 0xbfUL       /* Existing fields plus ClipActions, excluding clipDepth. */
#define FVM_SWF_CLIP_ACTION_MIN_VERSION 5U         /* First supported ClipActions file version. */
#define FVM_SWF_CLIP_ACTION_CONSTRUCT_VERSION 7U   /* Construct is gated by the strict file profile. */
#define FVM_SWF_CLIP_ACTION_SHORT_FLAG_BYTES 2U    /* UI16 flags before SWF6. */
#define FVM_SWF_CLIP_ACTION_LONG_FLAG_BYTES 4U     /* UI32 flags since SWF6. */
#define FVM_SWF_CLIP_ACTION_SHORT_FLAGS_VERSION 6U /* Earlier files use UI16, later ones UI32 flags. */
#define FVM_SWF_CLIP_ACTION_V5_MASK 0x001ffUL      /* Load through data; SWF6 interaction bits stay gated. */
#define FVM_SWF_CLIP_ACTION_V6_MASK 0x3ffffUL      /* All declared events except construct. */
#define FVM_SWF_CLIP_ACTION_KNOWN_MASK 0x7ffffUL   /* Reserved upper wire bits must remain zero. */
#define FVM_SWF_CLIP_ACTION_MAX_RECORDS 1024UL     /* Bound one placement's retained handler list. */
#define FVM_SWF_CLIP_ACTION_MAX_BYTES 4194304UL    /* Bound aggregate action bytes in one placement. */
#define FVM_SWF_CLIP_ACTION_HEX_PER_BYTE 2UL       /* Managed frame-code encoding uses two ASCII digits per byte. */
#define FVM_SWF_CLIP_ACTION_MAX_KEY 255U           /* Exact raw UI8 key selector; not a platform key code. */
#endif
