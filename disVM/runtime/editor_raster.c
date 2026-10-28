/* Rasterize detached native editor geometry without accessing either interpreter. */
#include "editor_raster.h"
#include <stdlib.h>
#include <string.h>


/* One painter-ordered owned contour; disjoint frames share none of this state. */
typedef struct FramePath
{
    FVM_Path *path;         /* Owned native contour, no font/VM references. */
    FVM_RasterColor color;  /* Copied straight-alpha ink. */
    struct FramePath *next; /* Owned next command. */
} FramePath;


/* A detached frame is mutable only during owner preparation or exclusive rendering. */
struct DVM_EditorFrame
{
    FVM_RasterSurface surface; /* Owned RGBA storage, allocated before submission. */
    FramePath *first;          /* First owned path in paint order. */
    FramePath *last;           /* Borrowed tail of the same owned list. */
    size_t paths;              /* Count checked against the command budget. */
    size_t verbs;              /* Total contour commands checked before transfer. */
};


/* Allocate bounded native storage; width/height are copied pixels, null signals failure. */
DVM_EditorFrame *dvm_editor_frame_create(unsigned int width, unsigned int height)
{
    DVM_EditorFrame *frame;
    if (!width || !height || width > DVM_EDITOR_FRAME_EXTENT || height > DVM_EDITOR_FRAME_EXTENT)
    {
        return NULL;
    }
    frame = calloc(1, sizeof(*frame));
    if (!frame)
    {
        return NULL;
    }
    frame->surface.width = width;
    frame->surface.height = height;
    frame->surface.stride = (size_t)width * 4;
    frame->surface.pixels = malloc(frame->surface.stride * height);
    if (!frame->surface.pixels)
    {
        free(frame);
        return NULL;
    }
    return frame;
}


/* Append an exclusively owned native path, retaining caller ownership on any failure. */
int dvm_editor_frame_path(DVM_EditorFrame *frame, FVM_Path *path, FVM_RasterColor color)
{
    FramePath *entry;
    size_t verbs;
    if (!frame || !path)
    {
        return 0;
    }
    verbs = fvm_path_count(path);
    if (frame->paths == DVM_EDITOR_FRAME_MAX_PATHS || verbs > DVM_EDITOR_FRAME_MAX_VERBS - frame->verbs)
    {
        return 0;
    }
    entry = malloc(sizeof(*entry));
    if (!entry)
    {
        return 0;
    }
    entry->path = path;
    entry->color = color;
    entry->next = NULL;
    if (frame->last)
    {
        frame->last->next = entry;
    }
    else
    {
        frame->first = entry;
    }
    frame->last = entry;
    frame->paths++;
    frame->verbs += verbs;
    return 1;
}


/* Return a borrowed completed surface; caller must enforce queue completion ownership. */
const FVM_RasterSurface *dvm_editor_frame_surface(const DVM_EditorFrame *frame)
{
    return frame ? &frame->surface : NULL;
}


/* Paint in order on a worker; every rasterizer and destination belongs to this frame. */
FVM_Result dvm_editor_frame_render(void *value)
{
    DVM_EditorFrame *frame = value;
    FramePath *entry;
    if (!frame)
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(frame->surface.pixels, 255, frame->surface.stride * frame->surface.height);
    for (entry = frame->first; entry; entry = entry->next)
    {
        FVM_Result result = fvm_path_draw(entry->path, &frame->surface, entry->color, 0, 0);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    return FVM_OK;
}


/* Reclaim only exclusively owned native resources; safe before or after rasterization. */
void dvm_editor_frame_destroy(void *value)
{
    DVM_EditorFrame *frame = value;
    if (frame)
    {
        FramePath *entry = frame->first;
        while (entry)
        {
            FramePath *next = entry->next;
            fvm_path_destroy(entry->path);
            free(entry);
            entry = next;
        }
        free(frame->surface.pixels);
        free(frame);
    }
}
