/* FlashVM: Transform property ordering, affine snapshots and stable transport identities. */
#ifndef FVM_TRANSFORM_CONSTANTS_H
#define FVM_TRANSFORM_CONSTANTS_H /* Private Transform schema guard. */
/* Property indices also select the native accessor specialization; preserve this ordering. */
#define FVM_TRANSFORM_PROPERTY_MATRIX 0U
#define FVM_TRANSFORM_PROPERTY_COLOR 1U
#define FVM_TRANSFORM_PROPERTY_WORLD_MATRIX 2U
#define FVM_TRANSFORM_PROPERTY_WORLD_COLOR 3U
#define FVM_TRANSFORM_PROPERTY_PIXEL_BOUNDS 4U
#define FVM_TRANSFORM_PROPERTY_COUNT 5U /* All readable native properties. */
#define FVM_TRANSFORM_WRITABLE_COUNT 2U /* Only local matrix and color have setters. */
/* Matrix snapshots use ActionScript's a,b,c,d,tx,ty positional constructor layout. */
#define FVM_TRANSFORM_MATRIX_A 0U
#define FVM_TRANSFORM_MATRIX_B 1U
#define FVM_TRANSFORM_MATRIX_C 2U
#define FVM_TRANSFORM_MATRIX_D 3U
#define FVM_TRANSFORM_MATRIX_TX 4U
#define FVM_TRANSFORM_MATRIX_TY 5U
#define FVM_TRANSFORM_MATRIX_COMPONENT_COUNT 6U
#define FVM_TRANSFORM_RECTANGLE_COMPONENT_COUNT 4U /* x,y,width,height snapshot arguments. */
#define FVM_TRANSFORM_COLOR_COMPONENT_COUNT 8U     /* RGBA multipliers followed by RGBA offsets. */
/* Existing serialized callback identities; never renumber when extending Transform. */
#define FVM_TRANSFORM_CALLBACK_CONSTRUCTOR 1400UL
#define FVM_TRANSFORM_CALLBACK_MOVIECLIP_GET 1401UL
#define FVM_TRANSFORM_CALLBACK_MOVIECLIP_SET 1402UL /* Appended setter; prior IDs stay stable. */
#define FVM_TRANSFORM_CALLBACK_GETTER_BASE 1410UL
#define FVM_TRANSFORM_CALLBACK_SETTER_BASE 1420UL
#endif
