/* Copyright (c) 2026 bhgv
 * EN: Queue copied IDE frames without owning the general terminal dispatcher.
 * RU: Передача копий кадров IDE без захвата общего диспетчера терминала.
 */
#include "frame.h"
#include "disvm/ui_frame.h"


/* EN/RU: See frame.h; validate before the transport accepts any bytes.
 * См. frame.h; проверка до принятия байтов транспортом. */
int styxbidiruisend(StyxBidirControl *control, unsigned long tag, const DVM_UIMessage *message)
{
    char bytes[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX]; /* Owned temporary encoding. */
    size_t size; /* Encoded bytes, set only after successful validation. */

    if (!control || !tag || tag > 0xffffffffUL ||
        dvm_ui_frame_encode(message, bytes, sizeof(bytes), &size) != FVM_OK)
    {
        return 0;
    }
    return styxbidircontrolsend(control, tag, bytes, size);
}


/* EN/RU: See frame.h; reject zero/control routes and never consume foreign records.
 * См. frame.h; tag0/control не является UI, чужая запись остаётся у диспетчера. */
int styxbidiruidecode(unsigned long expected, unsigned long tag, const void *bytes,
    size_t size, DVM_UIMessage *output)
{
    if (!expected || expected > 0xffffffffUL || !output)
    {
        return STYX_UI_INVALID;
    }
    if (tag != expected)
    {
        return STYX_UI_OTHER;
    }
    return dvm_ui_frame_decode(bytes, size, output) == FVM_OK ? STYX_UI_MESSAGE : STYX_UI_INVALID;
}
