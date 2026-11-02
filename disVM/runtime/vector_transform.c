/* Generic copied affine modifiers used by the local owner and raster snapshots. */
#include "vector_local.h"
#include "vector_transform.h"
#include <math.h>
#include <string.h>


/* Read/set an explicit endpoint/control. Buffers and point ordinal are validated
 * by the caller; no managed memory or external resource can be referenced. */
static void coordinate(DVM_VectorCommand *node, unsigned int point, long *x, long *y, int write)
{
    unsigned long *px = point == 0 ? &node->x : point == 1 ? &node->cx : &node->c2x;
    unsigned long *py = point == 0 ? &node->y : point == 1 ? &node->cy : &node->c2y;
    if (write)
    {
        *px = (unsigned long)*x;
        *py = (unsigned long)*y;
    }
    else
    {
        *x = (long)*px;
        *y = (long)*py;
    }
}


/* Apply one owner-selected modifier to detached baseline commands. Source
 * bounds include controls, matching the portable model's conservative frame.
 * For marquee only the last five supplied decoration commands change. */
int dvm_vector_transform(const DVM_VectorLocalPlan *p,
    const DVM_VectorCommand *source, const DVM_VectorCommand *overlay,
    long x, long y, DVM_VectorCommand *output, DVM_VectorCommand *decorated)
{
    long l = 1000;
    long t = 600;
    long r = 0;
    long b = 0;
    long ax;
    long ay;
    long bx;
    long by;
    long dx = x-(long)p->x;
    long dy = y-(long)p->y;
    double cosine = 1;
    double sine = 0;
    unsigned int pass;
    unsigned long i;
    memcpy(output,source,p->count*sizeof(*source));
    memcpy(decorated,overlay,p->overlay*sizeof(*overlay));
    if (p->mode == 5)
    {
        unsigned long at;
        if (p->overlay < p->count+5)
        {
            return 0;
        }
        l = x < (long)p->x ? x : (long)p->x;
        r = x > (long)p->x ? x : (long)p->x;
        t = y < (long)p->y ? y : (long)p->y;
        b = y > (long)p->y ? y : (long)p->y;
        at = p->overlay-5;
        for (i = 0; i < 4; ++i)
        {
            decorated[at+i].verb = i ? 1 : 0;
            decorated[at+i].x = (unsigned long)(i == 1 || i == 2 ? r : l);
            decorated[at+i].y = (unsigned long)(i >= 2 ? b : t);
        }
        memset(decorated+at+4,0,sizeof(*decorated));
        decorated[at+4].verb = 3;
        return 1;
    }
    for (i = 0; i < p->count; ++i)
    {
        DVM_VectorCommand n = source[i];
        unsigned int point;
        unsigned int count = n.verb == 3 ? 0 : n.verb == 4 ? 3 : n.verb == 2 ? 2 : 1;
        for (point = 0; point < count; ++point)
        {
            long px;
            long py;
            coordinate(&n,point,&px,&py,0);
            if (px < l)
            {
                l = px;
            }
            if (px > r)
            {
                r = px;
            }
            if (py < t)
            {
                t = py;
            }
            if (py > b)
            {
                b = py;
            }
        }
    }
    ax = p->handle&1 ? l : r;
    bx = p->handle&1 ? r : l;
    ay = p->handle&2 ? t : b;
    by = p->handle&2 ? b : t;
    if (p->mode == 3)
    {
        double cx = (l+r)/2.0;
        double cy = (t+b)/2.0;
        double angle = atan2(y-cy,x-cx)-atan2(p->y-cy,p->x-cx);
        cosine = cos(angle);
        sine = sin(angle);
    }
    for (pass = 0; pass < 2; ++pass)
    {
        unsigned long count = pass ? p->overlay : p->count;
        DVM_VectorCommand *nodes = pass ? decorated : output;
        for (i = 0; i < count; ++i)
        {
            unsigned int point;
            unsigned int points = nodes[i].verb == 3 ? 0 : nodes[i].verb == 4 ? 3 : nodes[i].verb == 2 ? 2 : 1;
            for (point = 0; point < points; ++point)
            {
                long px;
                long py;
                coordinate(nodes+i,point,&px,&py,0);
                if (p->mode == 1)
                {
                    px += dx;
                    py += dy;
                }
                else if (p->mode == 2)
                {
                    if (bx != ax)
                    {
                        px = ax+(px-ax)*(bx+dx-ax)/(bx-ax);
                    }
                    if (by != ay)
                    {
                        py = ay+(py-ay)*(by+dy-ay)/(by-ay);
                    }
                }
                else if (p->mode == 3)
                {
                    double cx = (l+r)/2.0;
                    double cy = (t+b)/2.0;
                    double tx = px-cx;
                    double ty = py-cy;
                    px = (long)(cx+tx*cosine-ty*sine);
                    py = (long)(cy+tx*sine+ty*cosine);
                }
                if (!pass && (px < 0 || px > 1000 || py < 0 || py > 600))
                {
                    return 0;
                }
                if (pass)
                {
                    px = px < 0 ? 0 : px > 1000 ? 1000 : px;
                    py = py < 0 ? 0 : py > 600 ? 600 : py;
                }
                coordinate(nodes+i,point,&px,&py,1);
            }
        }
    }
    return 1;
}
