/* Explicit bounds and identities for the three-frame standalone channel tutorial. */
#ifndef FVM_EXAMPLE_FRAME_PARTS_LIMITS_H /* Keep this example's constants local. */
#define FVM_EXAMPLE_FRAME_PARTS_LIMITS_H /* Guard the tutorial header. */
#define EXAMPLE_FRAME_WIDTH 32U /* Authored logical stage width, without native window scaling. */
#define EXAMPLE_FRAME_HEIGHT 32U /* Authored logical stage height. */
#define EXAMPLE_FRAME_BYTES (EXAMPLE_FRAME_WIDTH * EXAMPLE_FRAME_HEIGHT * 4U) /* Packed RGBA storage. */
#define EXAMPLE_FRAME_COUNT 3UL /* Demonstrate normal, smaller and larger profiles. */
#define EXAMPLE_FRAME_TIME_MS 40UL /* Tutorial presentation metadata; not an audio/video clock claim. */
#define EXAMPLE_FRAME_SESSION 1UL /* Trusted local reader granted READ by this host. */
#define EXAMPLE_FRAME_STREAM 7UL /* This demonstration's stream identity. */
#define EXAMPLE_FRAME_EPOCH 1UL /* Fresh local run; a remote reconnect must use a new epoch. */
#define EXAMPLE_FRAME_TAG 19UL /* Direction-bound bulk pixel type. */
#define EXAMPLE_FRAME_METADATA_TAG 20UL /* Direction-bound copied frame metadata. */
#define EXAMPLE_FRAME_METADATA_TYPE "flashvm.player.frame.metadata" /* Tutorial envelope for the existing metadata schema. */
#define EXAMPLE_FRAME_SLOT_SMALL 512UL /* Loss-tolerant optional bulk profile. */
#define EXAMPLE_FRAME_PART_SMALL 400UL /* Raw bytes leave conservative metadata reserves. */
#define EXAMPLE_FRAME_SLOT_NORMAL 1024UL /* Initial convenient profile. */
#define EXAMPLE_FRAME_PART_NORMAL 768UL /* Default raw part extent. */
#define EXAMPLE_FRAME_SLOT_LARGE 4096UL /* Larger profile for a reliable file/local channel. */
#define EXAMPLE_FRAME_PART_LARGE 4000UL /* Raw extent within the 4096-byte slot. */
#endif /* End explicitly chosen tutorial policy. */
