/* Private field indexes for canonical S-integer debugger payloads, not native struct offsets. */
#ifndef FLASHVM_RDEBUG_FORMAT_H
#define FLASHVM_RDEBUG_FORMAT_H   /* Guard the implementation schema indexes. */
#define RDEBUG_VERSION 0U         /* Common payload version field. */
#define RDEBUG_SNAP_STATE 1U      /* Callback state. */
#define RDEBUG_SNAP_FLAGS 2U      /* Label/stack truncation flags. */
#define RDEBUG_SNAP_STOP 3U       /* Stop incarnation. */
#define RDEBUG_SNAP_OFFSET 4U     /* AVM1 action offset. */
#define RDEBUG_SNAP_LINE 5U       /* Source line. */
#define RDEBUG_SNAP_FRAMES 6U     /* Cached frame count. */
#define RDEBUG_SNAP_CONTROLLER 7U /* Claimed session. */
#define RDEBUG_SNAP_FUNCTION 8U   /* Function byte count. */
#define RDEBUG_SNAP_SOURCE 9U     /* Source byte count. */
#define RDEBUG_FRAME_TRUNCATED 1U /* Frame label truncation Boolean. */
#define RDEBUG_FRAME_ENGINE 2U    /* Original engine. */
#define RDEBUG_FRAME_FORCED 3U    /* Entry-preservation Boolean. */
#define RDEBUG_FRAME_OFFSET 4U    /* AVM1 action offset. */
#define RDEBUG_FRAME_ARGC 5U      /* Supplied argument count. */
#define RDEBUG_FRAME_REGISTERS 6U /* Materialized register-slot count. */
#define RDEBUG_FRAME_FUNCTION 7U  /* Function byte count. */
#endif
