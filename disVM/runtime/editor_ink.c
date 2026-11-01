/* No live sample array crosses into Limbo: only frozen gestures may be copied. */
#include "editor_ink.h"
#include "editor_primitive.h"
#include <string.h>
#include <stdint.h>

typedef struct InkSlot
{
    unsigned long tool;                /*1 Brush,2 Rectangle,3 Ellipse,4 Line,5 Pen. */
    unsigned long token;               /* Monotonic nonreused value; zero denotes a free slot. */
    unsigned long widget;              /* Stable document presentation ID1..8. */
    unsigned long color;               /* Captured RGB24. */
    unsigned long width;               /* Captured logical diameter1..32. */
    unsigned int count;                /* Initialized packed logical points. */
    int frozen;                        /* Release seals the array before worker read. */
    int acknowledged;                  /* Hash is valid for eventual atomic preview replacement. */
    uint64_t hash;                     /* Complete expected wire identity; never document authority. */
    size_t size;                       /* Hash companion extent. */
    long points[DVM_INK_POINTS];       /* Sole-owner live trajectory. */
    char wire[DVM_INK_POINTS * 8 + 1]; /* Frozen worker copy source, native-owned. */
} InkSlot;

static InkSlot slots[DVM_INK_SLOTS]; /* Native UI-owner registry; workers see copies. */
static unsigned long next_token = 1; /* Refuse exhaustion instead of reusing identities. */

/* Locate an opaque live registry value; never reinterpret it as an address. */
static InkSlot *find(unsigned long token)
{
    unsigned int index;
    for (index = 0; token && index < DVM_INK_SLOTS; ++index)
    {
        if (slots[index].token == token)
        {
            return &slots[index];
        }
    }
    return NULL;
}

/* Bound a packed logical coordinate before storing/formatting any sample. */
static int valid(long point)
{
    return point >= 0 && point <= 601600;
}

/* Allocate only a free native slot. Captured style/first point are immutable. */
unsigned long dvm_ink_begin(unsigned long widget, unsigned long color, unsigned long width, long point)
{
    unsigned int index;
    if (widget < 1 || widget > 8 || color > 0xffffff || width < 1 || width > 32 || !valid(point) ||
        next_token > 2147483647UL)
    {
        return 0;
    }
    for (index = 0; index < DVM_INK_SLOTS; ++index)
    {
        if (!slots[index].token)
        {
            InkSlot *slot = &slots[index];
            memset(slot, 0, sizeof(*slot));
            slot->tool = 1;
            slot->token = next_token++;
            slot->widget = widget;
            slot->color = color;
            slot->width = width;
            slot->points[slot->count++] = point;
            return slot->token;
        }
    }
    return 0;
}

/* Append a changed point while space remains; caller controls capture ordering. */
int dvm_ink_point(unsigned long token, long point)
{
    InkSlot *slot = find(token);
    if (!slot || slot->frozen || !valid(point))
    {
        return 0;
    }
    if (slot->tool >= 2 && slot->tool <= 4)
    {
        slot->points[1] = point;
        slot->count = 2;
        return 1;
    }
    if (slot->points[slot->count - 1] == point)
    {
        return 1;
    }
    if (slot->count == DVM_INK_POINTS)
    {
        return 0;
    }
    slot->points[slot->count++] = point;
    return 1;
}

/* Freeze even a full retained trajectory. Never drop its earlier samples to fit
 * a release endpoint. Worker can still vectorize the complete retained prefix. */
int dvm_ink_end(unsigned long token, long point)
{
    InkSlot *slot = find(token);
    unsigned int index;
    if (!slot || slot->frozen || !valid(point))
    {
        return 0;
    }
    dvm_ink_point(token, point);
    for (index = 0; index < slot->count; ++index)
    {
        static const char digits[] = "0123456789abcdef";
        uint32_t value = (uint32_t)slot->points[index];
        unsigned int digit;
        for (digit = 0; digit < 8; ++digit)
        {
            slot->wire[index * 8 + digit] = digits[(value >> (28 - digit * 4)) & 15];
        }
    }
    slot->wire[slot->count * 8] = 0;
    slot->frozen = 1;
    return 1;
}

/* Native buffer is immutable after release and remains registered until commit. */
const char *dvm_ink_read(unsigned long token)
{
    InkSlot *slot = find(token);
    return slot && slot->frozen ? slot->wire : "";
}

/* Stable complete-stream content identity, paired with byte length on commit. */
static uint64_t fingerprint(const char *bytes, size_t size)
{
    uint64_t value = UINT64_C(14695981039346656037);
    size_t index;
    for (index = 0; index < size; ++index)
    {
        value = (value ^ (unsigned char)bytes[index]) * UINT64_C(1099511628211);
    }
    return value;
}

/* A cumulative worker result replaces all preceding captured strokes together. */
void dvm_ink_ack(unsigned long token, const char *drawing, size_t size)
{
    InkSlot *last = find(token);
    unsigned int index;
    uint64_t hash;
    if (!last || !last->frozen || !drawing || !size)
    {
        return;
    }
    hash = fingerprint(drawing, size);
    for (index = 0; index < DVM_INK_SLOTS; ++index)
    {
        InkSlot *slot = &slots[index];
        if (slot->token && slot->token <= token && slot->widget == last->widget && slot->frozen)
        {
            slot->acknowledged = 1;
            slot->hash = hash;
            slot->size = size;
        }
    }
}

/* Exact extent/hash gates visual retirement; no timer or worker-read clears ink. */
void dvm_ink_commit(unsigned long widget, const char *drawing, size_t size)
{
    unsigned int index;
    uint64_t hash = fingerprint(drawing, size);
    for (index = 0; index < DVM_INK_SLOTS; ++index)
    {
        InkSlot *slot = &slots[index];
        if (slot->widget == widget && slot->acknowledged && slot->size == size && slot->hash == hash)
        {
            slot->token = 0;
        }
    }
}

/* Emit in token order despite reuse of earlier registry slots. Geometry is
 * copied native data; constructing paths cannot allocate or inspect VM objects. */
int dvm_ink_preview(unsigned long widget, const FVM_Affine *world, double scale, DVM_InkEmit emit, void *user)
{
    unsigned long previous = 0;
    unsigned int ordinal;
    for (ordinal = 0; ordinal < DVM_INK_SLOTS; ++ordinal)
    {
        InkSlot *slot = NULL;
        unsigned int index;
        FVM_Path *path = NULL;
        FVM_Result result;
        FVM_RasterColor color;
        for (index = 0; index < DVM_INK_SLOTS; ++index)
        {
            if (slots[index].widget == widget && slots[index].token > previous &&
                (!slot || slots[index].token < slot->token))
            {
                slot = &slots[index];
            }
        }
        if (!slot)
        {
            break;
        }
        previous = slot->token;
        result = fvm_path_create(&path);
        for (index = 0; result == FVM_OK && (slot->tool == 1 || slot->tool == 5) && index < slot->count + (slot->count == 1); ++index)
        {
            FVM_PathCommand command;
            long point = slot->points[index < slot->count ? index : 0];
            memset(&command, 0, sizeof(command));
            command.verb = index ? FVM_PATH_LINE : FVM_PATH_MOVE;
            command.x[0] = 40 + (20 + point % 1001) * scale;
            command.y[0] = 28 + (20 + point / 1001) * scale;
            result = fvm_path_add(path, &command);
        }
        if (result == FVM_OK && slot->tool >= 2 && slot->tool <= 4)
        {
            result = dvm_primitive_path(path, slot->tool, slot->points[0], slot->points[slot->count - 1], scale);
        }
        if (result == FVM_OK)
        {
            result = fvm_path_transform(path, world);
        }
        color.r = (unsigned char)(slot->color >> 16);
        color.g = (unsigned char)(slot->color >> 8);
        color.b = (unsigned char)slot->color;
        color.a = 255;
        if (result != FVM_OK || !emit(user, path, color,
            (slot->tool >= 2 && slot->tool <= 4 && !slot->frozen ? -1.0 : 1.0) * slot->width * scale * world->xx))
        {
            fvm_path_destroy(path);
            return 0;
        }
        fvm_path_destroy(path);
    }
    return 1;
}

/* Cancel only one current capture; frozen unrelated preview layers survive. */
void dvm_ink_cancel(unsigned long token)
{
    InkSlot *slot = find(token);
    if (slot)
    {
        slot->token = 0;
    }
}

/* Called only after the UI/worker bridge is detached, before the next session. */
void dvm_ink_reset(void)
{
    memset(slots, 0, sizeof(slots));
}

/* Borrow only copied endpoint metadata for a lifecycle-triggered release. */
int dvm_ink_last(unsigned long token, unsigned long *widget, long *point)
{
    InkSlot *slot = find(token);
    if (!slot || slot->frozen)
    {
        return 0;
    }
    *widget = slot->widget;
    *point = slot->points[slot->count - 1];
    return 1;
}

/* A closed/reused document slot cannot inherit an earlier owner's raw preview. */
void dvm_ink_drop(unsigned long widget)
{
    unsigned int index;
    for (index = 0; index < DVM_INK_SLOTS; ++index)
    {
        if (slots[index].widget == widget)
        {
            slots[index].token = 0;
        }
    }
}

/* The sole native owner queries registry values before preparing a detached layer. */
int dvm_ink_has_preview(unsigned long widget)
{
    unsigned int index;
    for (index = 0; index < DVM_INK_SLOTS; index++)
    {
        if (slots[index].token && slots[index].widget == widget)
        {
            return 1;
        }
    }
    return 0;
}


/* Select copied tool semantics immediately after native capture admission. */
void dvm_ink_tool(unsigned long token, unsigned long tool)
{
    InkSlot *slot = find(token);
    if (slot && !slot->frozen && tool >= 1 && tool <= 5)
    {
        slot->tool = tool;
    }
}


/* Destination-dependent XOR cannot be cached as an ordinary alpha bitmap. */
int dvm_ink_has_xor(void)
{
    unsigned int index;
    for (index = 0; index < DVM_INK_SLOTS; ++index)
    {
        if (slots[index].token && !slots[index].frozen && slots[index].tool >= 2 && slots[index].tool <= 4)
        {
            return 1;
        }
    }
    return 0;
}
