/* Generic source-coordinate masks and overlay bindings, executed without Dis. */
#include "vector_local.h"
#include <stdlib.h>
#include <string.h>


/* A gesture owns all mutable scratch; only detached values reach raster workers. */
struct DVM_VectorLocal
{
    DVM_VectorLocalPlan plan; /* Immutable admitted modifiers. */
    DVM_VectorCommand source[DVM_VECTOR_LOCAL_NODES]; /* Immutable source baseline. */
    DVM_VectorCommand overlay[DVM_VECTOR_LOCAL_NODES]; /* Immutable overlay baseline. */
    DVM_VectorCommand output[DVM_VECTOR_LOCAL_NODES]; /* Private transactional source scratch. */
    DVM_VectorCommand decorated[DVM_VECTOR_LOCAL_NODES]; /* Private overlay scratch. */
    long x; /* Latest valid logical pointer X. */
    long y; /* Latest valid logical pointer Y. */
};


/* Return the number of explicit points for a validated path verb. */
static unsigned int points(unsigned long verb)
{
    return verb == 3 ? 0 : verb == 4 ? 3 : verb == 2 ? 2 : verb <= 1 ? 1 : 0;
}


/* Check one decoded command's scalar bounds and unused coordinates. */
static int command_valid(const DVM_VectorCommand *n)
{
    return n->verb <= 4 && n->x <= 1000 && n->y <= 600 && n->cx <= 1000 && n->cy <= 600 &&
        n->c2x <= 1000 && n->c2y <= 600 &&
        (n->verb == 2 || n->verb == 4 || (!n->cx && !n->cy)) &&
        (n->verb == 4 || (!n->c2x && !n->c2y)) && (n->verb != 3 || (!n->x && !n->y));
}


/* Validate complete extents before any output write or point-index dereference. */
int dvm_vector_local_valid(const DVM_VectorLocalPlan *p, const DVM_VectorCommand *source, const DVM_VectorCommand *overlay)
{
    unsigned long i;
    if (!p || !source || !overlay || p->count < 2 || p->count > DVM_VECTOR_LOCAL_NODES ||
        p->overlay < p->count || p->overlay > DVM_VECTOR_LOCAL_NODES || p->x > 1000 || p->y > 600 ||
        p->revision > 0x7fffffffUL || memcmp(source, overlay, p->count*sizeof(*source)))
    {
        return 0;
    }
    for (i = 0; i < p->count; ++i)
    {
        unsigned int count = points(source[i].verb);
        if (!command_valid(source+i) || p->masks[i] >= (1UL<<count))
        {
            return 0;
        }
    }
    for (i = 0; i < p->overlay; ++i)
    {
        unsigned long binding = p->bindings[i];
        unsigned long point = binding>>8;
        unsigned long flags = binding&255;
        if (!command_valid(overlay+i) || (i < p->count && binding))
        {
            return 0;
        }
        if (!binding)
        {
            continue;
        }
        if (!point || point > p->count*3 || (point-1)%3 >= points(source[(point-1)/3].verb) ||
            overlay[i].verb > 1 || (flags && (!(flags>>3) || (flags&7) < 1 || (flags&7) > 4)))
        {
            return 0;
        }
    }
    return 1;
}


/* Allocate a private copied baseline after validation; never retain borrowed arrays. */
DVM_VectorLocal *dvm_vector_local_create(const DVM_VectorLocalPlan *p, const DVM_VectorCommand *source, const DVM_VectorCommand *overlay)
{
    DVM_VectorLocal *e;
    if (!dvm_vector_local_valid(p, source, overlay) || !(e = malloc(sizeof(*e))))
    {
        return NULL;
    }
    e->plan = *p;
    memcpy(e->source, source, p->count*sizeof(*source));
    memcpy(e->overlay, overlay, p->overlay*sizeof(*overlay));
    e->x = (long)p->x;
    e->y = (long)p->y;
    return e;
}


/* Compare only initialized plan ranges and canonical baseline commands. */
int dvm_vector_local_matches(const DVM_VectorLocal *e, const DVM_VectorLocalPlan *p, const DVM_VectorCommand *source, const DVM_VectorCommand *overlay)
{
    return e && dvm_vector_local_valid(p,source,overlay) && e->plan.source == p->source &&
        e->plan.revision == p->revision && e->plan.x == p->x && e->plan.y == p->y &&
        e->plan.count == p->count && e->plan.overlay == p->overlay &&
        !memcmp(e->plan.masks,p->masks,p->count*sizeof(*p->masks)) &&
        !memcmp(e->plan.bindings,p->bindings,p->overlay*sizeof(*p->bindings)) &&
        !memcmp(e->source,source,p->count*sizeof(*source)) &&
        !memcmp(e->overlay,overlay,p->overlay*sizeof(*overlay));
}


/* Read one explicit command point without exposing a native pointer externally. */
static void point_get(const DVM_VectorCommand *n, unsigned int point, long *x, long *y)
{
    *x = (long)(point == 0 ? n->x : point == 1 ? n->cx : n->c2x);
    *y = (long)(point == 0 ? n->y : point == 1 ? n->cy : n->c2y);
}


/* Replace a caller-validated explicit point in private output scratch. */
static void point_set(DVM_VectorCommand *n, unsigned int point, long x, long y)
{
    if (point == 0)
    {
        n->x = (unsigned long)x;
        n->y = (unsigned long)y;
    }
    else if (point == 1)
    {
        n->cx = (unsigned long)x;
        n->cy = (unsigned long)y;
    }
    else
    {
        n->c2x = (unsigned long)x;
        n->c2y = (unsigned long)y;
    }
}


/* Clamp a derived marker corner after applying its original signed radius. */
static long clamp(long value, long maximum)
{
    return value < 0 ? 0 : value > maximum ? maximum : value;
}


/* Apply selected source-point translation, closed-endpoint aliases and copied
 * decoration bindings transactionally. No path picking or UI policy lives here. */
int dvm_vector_local_move(DVM_VectorLocal *e, long x, long y, DVM_VectorCommand *source, DVM_VectorCommand *overlay)
{
    unsigned long i;
    unsigned long first = 0;
    long dx;
    long dy;
    if (!e || !source || !overlay || x < 0 || x > 1000 || y < 0 || y > 600)
    {
        return 0;
    }
    dx = x-(long)e->plan.x;
    dy = y-(long)e->plan.y;
    memcpy(e->output,e->source,e->plan.count*sizeof(*source));
    for (i = 0; i < e->plan.count; ++i)
    {
        unsigned int p;
        for (p = 0; p < points(e->source[i].verb); ++p)
        {
            if (e->plan.masks[i] & (1UL<<p))
            {
                long px;
                long py;
                point_get(e->source+i,p,&px,&py);
                px += dx;
                py += dy;
                if (px < 0 || px > 1000 || py < 0 || py > 600)
                {
                    return 0;
                }
                point_set(e->output+i,p,px,py);
            }
        }
        if (e->source[i].verb == 0)
        {
            first = i;
        }
        if (e->source[i].verb == 4 && i+1 < e->plan.count && e->source[i+1].verb == 3 &&
            e->source[i].x == e->source[first].x && e->source[i].y == e->source[first].y &&
            ((e->plan.masks[i]|e->plan.masks[first])&1))
        {
            long px = (long)e->source[i].x+dx;
            long py = (long)e->source[i].y+dy;
            if (px < 0 || px > 1000 || py < 0 || py > 600)
            {
                return 0;
            }
            point_set(e->output+i,0,px,py);
            point_set(e->output+first,0,px,py);
        }
    }
    memcpy(e->decorated,e->overlay,e->plan.overlay*sizeof(*overlay));
    memcpy(e->decorated,e->output,e->plan.count*sizeof(*source));
    for (i = e->plan.count; i < e->plan.overlay; ++i)
    {
        unsigned long binding = e->plan.bindings[i];
        unsigned long point = binding>>8;
        unsigned long flags = binding&255;
        if (binding)
        {
            long px;
            long py;
            point_get(e->output+(point-1)/3,(unsigned int)((point-1)%3),&px,&py);
            if (flags)
            {
                long radius = (long)(flags>>3);
                unsigned long corner = flags&7;
                px += corner == 1 || corner == 4 ? -radius : radius;
                py += corner == 1 || corner == 2 ? -radius : radius;
            }
            point_set(e->decorated+i,0,clamp(px,1000),clamp(py,600));
        }
    }
    memcpy(source,e->output,e->plan.count*sizeof(*source));
    memcpy(overlay,e->decorated,e->plan.overlay*sizeof(*overlay));
    e->x = x;
    e->y = y;
    return 1;
}


/* Return the last admitted pointer; callers supply valid scalar output storage. */
void dvm_vector_local_point(const DVM_VectorLocal *e, long *x, long *y)
{
    *x = e->x;
    *y = e->y;
}


/* Restore copied geometry after Escape/focus cancellation without allocation. */
void dvm_vector_local_restore(const DVM_VectorLocal *e, DVM_VectorCommand *source, DVM_VectorCommand *overlay)
{
    memcpy(source,e->source,e->plan.count*sizeof(*source));
    memcpy(overlay,e->overlay,e->plan.overlay*sizeof(*overlay));
}


/* Release the exclusively owned baseline after commit/cancel or owner teardown. */
void dvm_vector_local_destroy(DVM_VectorLocal *e)
{
    free(e);
}
