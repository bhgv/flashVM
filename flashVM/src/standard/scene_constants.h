/* FlashVM: decoded scene-control flags, identities and color representation bounds. */
#ifndef FVM_SCENE_CONSTANTS_H
#define FVM_SCENE_CONSTANTS_H              /* Private C89 scene-metadata guard. */
#define FVM_SCENE_PLACE_MOVE 0x01UL        /* Modify/replace an existing authored depth. */
#define FVM_SCENE_PLACE_CHARACTER 0x02UL   /* A symbol definition is supplied. */
#define FVM_SCENE_PLACE_MATRIX 0x04UL      /* Optional affine transform. */
#define FVM_SCENE_PLACE_COLOR 0x08UL       /* Optional color transform. */
#define FVM_SCENE_PLACE_RATIO 0x10UL       /* Optional morph ratio. */
#define FVM_SCENE_PLACE_NAME 0x20UL        /* Initial instance-name assignment. */
#define FVM_SCENE_PLACE_MODE_MASK 0x03UL   /* Move/character operation selectors. */
#define FVM_SCENE_MAX_CHARACTER_ID 65535UL /* UI16 symbol identity. */
#define FVM_SCENE_MAX_TOKEN 0xffffffffUL   /* Decoded authored instance identity. */
#define FVM_SCENE_SYMBOL_SHAPE 2.0         /* Private normalized vector-symbol discriminator. */
#define FVM_SCENE_MAX_RGB 0xffffffUL       /* Three opaque eight-bit color channels. */
#define FVM_SCENE_RED_SHIFT 16U            /* Red byte in packed RGB. */
#define FVM_SCENE_GREEN_SHIFT 8U           /* Green byte in packed RGB. */
#define FVM_SCENE_OPAQUE_ALPHA 255U        /* Full opacity in the host raster ABI. */
#endif
