/* Fast immutable-path preview: prepare a nib once, blit along a sampled segment. */
#include "editor_stamp.h"
#include <math.h>
#include <string.h>

#define STAMP_EXTENT 68 /* Width<=64 plus antialias/rounding margin. */


/* Read-only eligibility preflight; unsupported paths must remain unmodified.
 * Return one for bounded opaque polylines, zero for normal vector fallback. */
static int eligible(const FVM_Path *path, FVM_RasterColor color, double width)
{
    size_t index;
    size_t count = fvm_path_count(path);
    if (color.a != 255 || !(width >= 1 && width <= 64) || !count || count > 16384)
    {
        return 0;
    }
    for (index = 0; index < count; ++index)
    {
        FVM_PathCommand command;
        if (fvm_path_get(path, index, &command) != FVM_OK ||
            (command.verb != FVM_PATH_MOVE && command.verb != FVM_PATH_LINE) ||
            (index == 0 && command.verb != FVM_PATH_MOVE))
        {
            return 0;
        }
    }
    return 1;
}


/* Blit one borrowed coverage nib into a clipped straight-alpha RGBA surface.
 * Integer centre is the only preview quantization; opaque interior uses stores.
 * Caller owns surface exclusively; no allocation or path construction occurs. */
static void blit(FVM_RasterSurface *surface,
                 const unsigned char *mask,
                 int extent,
                 int centre_x,
                 int centre_y,
                 FVM_RasterColor color)
{
    int x;
    int y;
    int origin_x = centre_x - extent / 2;
    int origin_y = centre_y - extent / 2;
    for (y = 0; y < extent; ++y)
    {
        int py = origin_y + y;
        if (py < 0 || py >= (int)surface->height)
        {
            continue;
        }
        for (x = 0; x < extent; ++x)
        {
            int px = origin_x + x;
            unsigned int alpha = mask[y * extent + x];
            unsigned char *out;
            unsigned int old_alpha;
            unsigned int total;
            unsigned int channel;
            unsigned int ink[3];
            if (!alpha || px < 0 || px >= (int)surface->width)
            {
                continue;
            }
            out = surface->pixels + (size_t)py * surface->stride + (size_t)px * 4;
            ink[0] = color.r;
            ink[1] = color.g;
            ink[2] = color.b;
            old_alpha = out[3];
            if (alpha == 255 || (old_alpha == 255 && out[0] == color.r && out[1] == color.g && out[2] == color.b))
            {
                out[0] = color.r;
                out[1] = color.g;
                out[2] = color.b;
                out[3] = 255;
                continue;
            }
            if (old_alpha == 255)
            {
                out[0] = (unsigned char)((color.r * alpha + out[0] * (255 - alpha) + 127) / 255);
                out[1] = (unsigned char)((color.g * alpha + out[1] * (255 - alpha) + 127) / 255);
                out[2] = (unsigned char)((color.b * alpha + out[2] * (255 - alpha) + 127) / 255);
                continue;
            }
            total = alpha * 255 + old_alpha * (255 - alpha);
            for (channel = 0; channel < 3; ++channel)
            {
                out[channel] = (unsigned char)((ink[channel] * alpha * 255 + out[channel] * old_alpha * (255 - alpha) +
                                                total / 2) /
                                               total);
            }
            out[3] = (unsigned char)((total + 127) / 255);
        }
    }
}


/* Clip one parameter interval against an axis-aligned nib-expanded viewport.
 * Inputs are copied scalars; outputs belong to the caller. No pixels are touched. */
static int edge(double p, double q, double *first, double *last)
{
    double t;
    if (p == 0)
    {
        return q >= 0;
    }
    t = q / p;
    if (p < 0)
    {
        if (t > *last)
        {
            return 0;
        }
        if (t > *first)
        {
            *first = t;
        }
    }
    else
    {
        if (t < *first)
        {
            return 0;
        }
        if (t < *last)
        {
            *last = t;
        }
    }
    return 1;
}


/* Stamp only the visible part of a segment, including both endpoints. Major-axis
 * spacing<=1 pixel prevents gaps; clipping avoids long offscreen walks. */
static void segment(FVM_RasterSurface *surface,
                    const unsigned char *mask,
                    int extent,
                    double ax,
                    double ay,
                    double bx,
                    double by,
                    FVM_RasterColor color)
{
    double dx = bx - ax;
    double dy = by - ay;
    double first = 0;
    double last = 1;
    double margin = extent;
    int steps;
    int index;
    if (!edge(-dx, ax + margin, &first, &last) || !edge(dx, surface->width + margin - ax, &first, &last) ||
        !edge(-dy, ay + margin, &first, &last) || !edge(dy, surface->height + margin - ay, &first, &last))
    {
        return;
    }
    bx = ax + dx * last;
    by = ay + dy * last;
    ax += dx * first;
    ay += dy * first;
    dx = bx - ax;
    dy = by - ay;
    steps = (int)ceil(fmax(fabs(dx), fabs(dy)));
    for (index = 0; index <= steps; ++index)
    {
        double t = steps ? (double)index / steps : 0;
        blit(surface, mask, extent, (int)floor(ax + dx * t + 0.5), (int)floor(ay + dy * t + 0.5), color);
    }
}


/* Precompute a sampled coverage nib once per detached stroke/tile. The fallback
 * preserves filled/curved/transparent vector paths. This preview does not fit,
 * vectorize or mutate document geometry; Limbo owns those operations on release. */
FVM_Result dvm_editor_stamp_draw(
    const FVM_Path *path, FVM_RasterSurface *surface, FVM_RasterColor color, double width, int even_odd)
{
    unsigned char mask[STAMP_EXTENT * STAMP_EXTENT];
    int extent;
    int x;
    int y;
    size_t index;
    double ax = 0;
    double ay = 0;
    if (!path || !surface || !surface->pixels || !surface->width || !surface->height ||
        surface->stride < (size_t)surface->width * 4 || surface->width > 4096 || surface->height > 4096)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!eligible(path, color, width))
    {
        return fvm_path_draw(path, surface, color, width, even_odd);
    }
    extent = ((int)ceil(width) + 3) & ~1;
    memset(mask, 0, sizeof(mask));
    for (y = 0; y < extent; ++y)
    {
        for (x = 0; x < extent; ++x)
        {
            double dx = x - extent / 2 + 0.5;
            double dy = y - extent / 2 + 0.5;
            double coverage = width * 0.5 + 0.5 - sqrt(dx * dx + dy * dy);
            if (coverage > 0)
            {
                mask[y * extent + x] = (unsigned char)(coverage >= 1 ? 255 : floor(coverage * 255 + 0.5));
            }
        }
    }
    for (index = 0; index < fvm_path_count(path); ++index)
    {
        FVM_PathCommand command;
        if (fvm_path_get(path, index, &command) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (command.verb == FVM_PATH_LINE)
        {
            segment(surface, mask, extent, ax, ay, command.x[0], command.y[0], color);
        }
        ax = command.x[0];
        ay = command.y[0];
    }
    return FVM_OK;
}
