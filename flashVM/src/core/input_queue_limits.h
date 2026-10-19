/* Internal copied-queue variants; never serialized or passed as platform event IDs. */
#ifndef FVM_CORE_INPUT_QUEUE_LIMITS_H
#define FVM_CORE_INPUT_QUEUE_LIMITS_H /* Guard private event discriminants. */
#define FVM_QUEUED_GESTURE 1U /* Recognized gesture delivered through the host callback. */
#define FVM_QUEUED_MOUSE 2U /* Primary-pointer transition delivered through Mouse listeners. */
#define FVM_QUEUED_KEYBOARD 3U /* Complete key snapshot plus event-time pointer position. */
#endif
