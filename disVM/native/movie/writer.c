/* Copyright (c) 2026 bhgv
 * EN: Native outbox drain with bounded staging, exact partial acknowledgements and sticky failure.
 * RU: Native outbox drain с bounded staging, exact partial acknowledgements и sticky failure.
 */
#include "writer.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Exclusively owned by one native I/O caller; source observers only touch its outbox.
 * Исключительное владение одного native I/O caller; observers касаются только outbox. */
struct StyxMovieWriter
{
    StyxEvents *outbox; /* Owned bounded autonomous journal subscription. */
    unsigned char bytes[MOVIE_OFFER_FRAME*2]; /* One complete declaration/offer pair, never interleaved. */
    size_t size; /* Current staged extent. */
    size_t sent; /* Confirmed prefix, never replayed. */
    int last; /* Staged pair is CLOSED; EOF only after all bytes are confirmed. */
    int ended; /* Sticky successful EOF. */
    int failed; /* Sticky refusal after uncertain, malformed or failing transport. */
};


/* EN/RU: Create initialized scalar result for native stop/status; no ownership or I/O.
 * Создать scalar result для native stop/status, без ownership/I/O. */
static DVM_IoResult result(FVM_Result status, DVM_IoStop stop)
{
    DVM_IoResult out;
    memset(&out, 0, sizeof(out));
    out.status = status;
    out.stop = stop;
    out.flags = DVM_IO_PREFIX;
    out.probe_status = FVM_UNSUPPORTED;
    return out;
}


/* EN/RU: Validate our staged codec output and determine CLOSED from those exact bytes.
 * Never infer retirement from a later source snapshot while READY bytes are partly sent.
 * Проверить staged output, определить CLOSED по тем же bytes, не по более новому snapshot. */
static int staged(StyxMovieWriter *writer)
{
    FVM_SRecord first, offer;
    size_t prefix, suffix;
    if (fvm_srecord_read(writer->bytes, writer->size, sizeof(writer->bytes), &first, &prefix) != FVM_S_OK ||
        first.tag || prefix >= writer->size ||
        fvm_srecord_read(writer->bytes+prefix, writer->size-prefix, sizeof(writer->bytes), &offer, &suffix) != FVM_S_OK ||
        prefix+suffix != writer->size || !offer.tag || !offer.size ||
        (offer.payload[0] != MOVIE_OFFER_READY && offer.payload[0] != MOVIE_OFFER_CLOSED))
    {
        return 0;
    }
    writer->last = offer.payload[0] == MOVIE_OFFER_CLOSED;
    return 1;
}


/* EN/RU: See writer.h; all setup completes before returning an owned serializer.
 * См. writer.h; setup завершается до выдачи owned serializer. */
StyxMovieWriter *styxmoviewriter(DVM_MovieOffer *source, unsigned long tag)
{
    StyxMovieWriter *writer = calloc(1, sizeof(*writer));
    if (!writer)
    {
        return NULL;
    }
    writer->outbox = styxmovieofferwatch(source, tag);
    if (!writer->outbox)
    {
        free(writer);
        return NULL;
    }
    return writer;
}


/* EN/RU: See writer.h; ACK validation precedes prefix advancement and buffer reuse.
 * См. writer.h; ACK validation до prefix advancement и buffer reuse. */
DVM_IoResult styxmoviewriterstep(StyxMovieWriter *writer, StyxMovieWrite write, void *state)
{
    if (!writer || !write)
    {
        return result(FVM_INVALID_ARGUMENT, DVM_IO_FAILURE);
    }
    if (writer->failed)
    {
        return result(FVM_ERROR, DVM_IO_FAILURE);
    }
    if (writer->ended)
    {
        return result(FVM_OK, DVM_IO_EOF);
    }
    if (writer->sent == writer->size)
    {
        int read = styxeventsread(writer->outbox, writer->bytes, sizeof(writer->bytes), &writer->size);
        writer->sent = 0;
        if (read == STYX_EVENT_EMPTY)
        {
            return result(FVM_ERROR, DVM_IO_AGAIN);
        }
        if (read != STYX_EVENT_RECORD || !staged(writer))
        {
            writer->failed = 1;
            return result(FVM_ERROR, DVM_IO_INVALID);
        }
    }
    size_t remaining = writer->size-writer->sent;
    DVM_IoResult out = result(FVM_ERROR, DVM_IO_INVALID);
    write(state, writer->bytes+writer->sent, remaining, &out);
    if (out.count > remaining || !(out.flags & DVM_IO_PREFIX) ||
        out.status < FVM_EXCEPTION || out.status > FVM_OK ||
        out.stop < DVM_IO_NONE || out.stop >= DVM_IO_INVALID || out.stop == DVM_IO_EOF ||
        (out.flags & ~DVM_IO_FLAGS) ||
        (out.stop == DVM_IO_NONE && out.status != FVM_OK))
    {
        writer->failed = 1;
        return result(FVM_ERROR, DVM_IO_INVALID);
    }
    out.flags &= ~DVM_IO_RECORD;
    writer->sent += out.count;
    if ((out.flags & DVM_IO_UNCERTAIN) ||
        (out.stop != DVM_IO_NONE && out.stop != DVM_IO_AGAIN) ||
        (!out.count && out.stop == DVM_IO_NONE))
    {
        writer->failed = 1;
        if ((out.flags & DVM_IO_UNCERTAIN) && (out.stop == DVM_IO_NONE || out.stop == DVM_IO_AGAIN))
        {
            out.status = FVM_ERROR;
            out.stop = DVM_IO_FAILURE;
        }
        if (!out.count && out.stop == DVM_IO_NONE)
        {
            out.status = FVM_ERROR;
            out.stop = DVM_IO_NO_PROGRESS;
        }
        return out;
    }
    if (writer->sent == writer->size)
    {
        out.flags |= DVM_IO_RECORD;
        writer->ended = writer->last;
    }
    return out;
}


/* EN/RU: See writer.h; descriptor remains owned by the retained outbox.
 * См. writer.h; descriptor принадлежит удержанному outbox. */
int styxmoviewriterfd(StyxMovieWriter *writer)
{
    return writer && !writer->ended && !writer->failed ? styxeventsfd(writer->outbox) : -1;
}


/* EN/RU: See writer.h; observer callbacks never mutate serializer offsets.
 * См. writer.h; observer callbacks не меняют serializer offsets. */
size_t styxmoviewriterpending(StyxMovieWriter *writer)
{
    return writer && !writer->failed && !writer->ended ? writer->size-writer->sent : 0;
}


/* EN/RU: See writer.h; admitted source callbacks hold independent encoder pins.
 * См. writer.h; admitted source callbacks удерживают independent encoder pins. */
void styxmoviewriterfree(StyxMovieWriter *writer)
{
    if (writer)
    {
        styxeventsfree(writer->outbox);
        free(writer);
    }
}
