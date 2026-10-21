/* Small native fixture dimensions, separate from the protocol's bounded object maximum. */
#ifndef FVM_EXAMPLE_MEDIA_LIMITS_H /* Avoid repeated example constants. */
#define FVM_EXAMPLE_MEDIA_LIMITS_H /* Guard this tutorial's local geometry. */
#define EXAMPLE_MEDIA_PARTS 3U /* Three slots make the damaged middle part observable. */
#define EXAMPLE_MEDIA_TAIL 17U /* Final short part proves exact extent handling. */
#define EXAMPLE_MEDIA_BYTES (2U * FVM_SMEDIA_PAYLOAD + EXAMPLE_MEDIA_TAIL) /* Source extent. */
#define EXAMPLE_MEDIA_TAG 19UL /* Direction-local tag already agreed by this in-memory host. */
#define EXAMPLE_CONTROL_TAG 44UL /* Independent compact control type for this example only. */
#define EXAMPLE_MEDIA_CEILING 4096UL /* Local advertised bulk-buffer capacity, before negotiation. */
#define EXAMPLE_NEGOTIATION_ID 1UL /* First monotonically increasing request in this fresh session. */
#endif /* End example geometry. */
