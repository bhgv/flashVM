/* Copyright (c) 2026 bhgv
 * EN: Desktop binding setup and vector geometry; included by the owner translation unit.
 * RU: Привязки рабочего стола и векторная геометрия; включается в translation unit владельца.
 */
/* Create a genuine MovieClip through the standard Flash method and parent edge.
 * parent/name/depth are borrowed; returned object stays rooted through that edge. */
static FVM_Object *child(FVM_Object *parent, const char *name, unsigned int depth)
{
    FVM_Value args[2];
    FVM_Value result;
    fvm_value_string(&args[0], name);
    fvm_value_number(&args[1], depth);
    if (fvm_object_call(owner, parent, "createEmptyMovieClip", 2, args, &result) != FVM_OK ||
        result.type != FVM_VALUE_OBJECT)
    {
        return NULL;
    }
    return result.object;
}

/* Read admitted rail geometry from the complete copied scene. A scene without
 * a right rail owns the whole canvas. No theme constant or VM pointer escapes. */
static unsigned long right_extent(void)
{
    unsigned int index;
    for (index = 0; index < received; index++)
    {
        if (items[index].id == 12)
        {
            return items[index].width <= width ? items[index].width : width;
        }
    }
    return 0;
}

/* Append one value rectangle to a retained MovieClip without clearing siblings.
 * Coordinates/extents/color are copied pixels; return zero on a VM call failure.
 * The caller owns the clip's context and serializes all managed access. */
static int rectangle(FVM_Object *clip, double left, double top, double wide, double high)
{
    FVM_Value args[2];
    FVM_Value ignored;
    double x[5] = {left, left + wide, left + wide, left, left};
    double y[5] = {top, top, top + high, top + high, top};
    int index;
    fvm_value_number(&args[0], 0x95b9d1UL);
    fvm_value_number(&args[1], 100);
    if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (index = 0; index < 5; ++index)
    {
        fvm_value_number(&args[0], x[index]);
        fvm_value_number(&args[1], y[index]);
        if (fvm_object_call(owner, clip, index ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}

/* Initialize owner-local scene state and linked AS bytecode once. The root pins
 * all child capabilities; partial startup is retired by destroy. Return success. */
int dvm_desktop_bind(FVM_Context *context, const unsigned char *font, size_t size, int replay)
{
    unsigned int index;
    FVM_Result result;
    owner = context;
    trace = replay;
    if (!owner || !font || !size)
    {
        return 0;
    }
    result = fvm_load_memory(owner, ide_view_swf, sizeof(ide_view_swf));
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop load: %d\n", result);
        return 0;
    }
    result = fvm_standard_install(owner);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop standard: %d\n", result);
        return 0;
    }
    result = fvm_movieclip_create(owner, "Desktop", 1, &root);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop root: %d\n", result);
        return 0;
    }
    result = fvm_gc_pin(owner, root);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop root pin: %d\n", result);
        return 0;
    }
    result = fvm_font_read(font, size, 0, 11, &face);
    if (result != FVM_OK)
    {
        fprintf(stderr, "desktop face: %d\n", result);
        return 0;
    }
    groups[0] = root;
    for (index = 1; index <= 4; ++index)
    {
        char name[24];
        snprintf(name, sizeof(name), "layer%u", index);
        groups[index] = child(root, name, 10 + index * 10);
        if (!groups[index])
        {
            fprintf(stderr, "desktop group %u\n", index);
            return 0;
        }
    }
    return 1;
}

/* Parse a complete bounded private command stream. Every extent is checked
 * before dereference; no partial stream is paintable and no SVG/XML is parsed. */
static int vector_parse(VectorScene *scene)
{
    size_t at = 56;
    unsigned long shapes;
    size_t stride;
    int cubic;
    int local;
    unsigned int total = 0;
    unsigned int i;
    if (scene->used < 56 || !word(scene->bytes, &scene->tool) || !word(scene->bytes + 8, &scene->color) ||
        !word(scene->bytes + 16, &scene->width) || !word(scene->bytes + 24, &scene->fill) ||
        !word(scene->bytes + 32, &scene->undo) || !word(scene->bytes + 40, &scene->redo) ||
        scene->color > 0xffffffUL)
    {
        return 0;
    }
    int extended = (scene->tool & 0x200000UL) != 0;
    scene->tool &= ~0x200000UL;
    int compact = (scene->tool & 0x100000UL) != 0;
    scene->tool &= ~0x100000UL;
    int armed = (scene->tool & 0x80000UL) != 0;
    scene->tool &= ~0x80000UL;
    local = (scene->tool & 0x40000UL) != 0;
    if ((armed || compact || extended) && !local)
    {
        return 0;
    }
    scene->tool &= ~0x40000UL;
    memset(&scene->local,0,sizeof(scene->local));
    cubic = (scene->tool & 0x20000UL) != 0;
    scene->tool &= ~0x20000UL;
    stride = cubic ? 56 : 40;
    scene->zoom = 100;
    scene->center_x = 500;
    scene->center_y = 300;
    if (scene->tool & 0x10000UL)
    {
        scene->tool &= ~0x10000UL;
        if (scene->used < 80 || !word(scene->bytes + 48, &scene->zoom) ||
            !word(scene->bytes + 56, &scene->center_x) || !word(scene->bytes + 64, &scene->center_y) ||
            scene->zoom < 25 || scene->zoom > 1600 || scene->center_x > 1000 || scene->center_y > 600)
        {
            return 0;
        }
        at += 24;
    }
    if (local)
    {
        DVM_VectorLocalPlan *plan = &scene->local;
        unsigned long n;
        size_t start = at-8;
        if (scene->used-start < 56 || (scene->tool != 7 && scene->tool != 6) ||
            !word(scene->bytes+start,&plan->source) || !word(scene->bytes+start+8,&plan->revision) ||
            !word(scene->bytes+start+16,&plan->x) || !word(scene->bytes+start+24,&plan->y) ||
            !word(scene->bytes+start+32,&plan->count) || !word(scene->bytes+start+40,&plan->overlay) ||
            plan->source >= 32 || plan->revision > 0x7fffffffUL || plan->x > 1000 || plan->y > 600 ||
            plan->count < 2 || plan->count > DVM_VECTOR_LOCAL_NODES || plan->overlay < plan->count ||
            plan->overlay > DVM_VECTOR_LOCAL_NODES ||
            plan->count+plan->overlay > (scene->used-start-56)/8)
        {
            return 0;
        }
        start += 48;
        if (extended)
        {
            if (scene->used-start < 24 || !word(scene->bytes+start,&plan->mode) ||
                !word(scene->bytes+start+8,&plan->handle))
            {
                return 0;
            }
            start += 16;
        }
        if (scene->used-start < 8 || plan->count+plan->overlay > (scene->used-start-8)/8 ||
            (scene->tool == 6 && (plan->mode < 1 || plan->mode > 3)) ||
            (scene->tool == 7 && plan->mode != 0 && plan->mode != 5))
        {
            return 0;
        }
        for (n = 0; n < plan->count; ++n, start += 8)
        {
            if (!word(scene->bytes+start,&plan->masks[n]))
            {
                return 0;
            }
        }
        for (n = 0; n < plan->overlay; ++n, start += 8)
        {
            if (!word(scene->bytes+start,&plan->bindings[n]))
            {
                return 0;
            }
        }
        at = start+8;
    }
    if (!word(scene->bytes + at - 8, &shapes) || scene->tool < 1 || scene->tool > 8 ||
        scene->width < 1 || scene->width > 32 || scene->fill > 3 ||
        ((scene->fill & 2) && (scene->tool != 4 || shapes == 0)) || scene->undo > 16 || scene->redo > 16 ||
        shapes > VECTOR_SHAPES)
    {
        return 0;
    }
    for (i = 0; i < shapes; ++i)
    {
        VectorShape *shape = &scene->shapes[i];
        unsigned long count;
        unsigned int j;
        if (scene->used - at < 32 || !word(scene->bytes + at, &shape->color) ||
            !word(scene->bytes + at + 8, &shape->width) || !word(scene->bytes + at + 16, &shape->fill) ||
            !word(scene->bytes + at + 24, &count) || shape->color > 0xffffffUL || shape->width > 32 ||
            shape->fill > 1 || (!shape->width && !shape->fill) || count < 2 || count > 4096 ||
            count > VECTOR_NODES - total)
        {
            return 0;
        }
        at += 32;
        shape->first = total;
        shape->count = (unsigned int)count;
        if (compact && i+1 == shapes)
        {
            DVM_VectorLocalPlan *plan = &scene->local;
            if (plan->source >= i || scene->shapes[plan->source].count != plan->count ||
                plan->overlay != count || !dvm_vector_local_decorate(plan,
                    scene->nodes+scene->shapes[plan->source].first,scene->nodes+total))
            {
                return 0;
            }
            total += (unsigned int)count;
            continue;
        }
        if (count > (scene->used - at) / stride)
        {
            return 0;
        }
        for (j = 0; j < count; ++j)
        {
            VectorNode *node = &scene->nodes[total++];
            if (!word(scene->bytes + at, &node->verb) || !word(scene->bytes + at + 8, &node->x) ||
                !word(scene->bytes + at + 16, &node->y) || !word(scene->bytes + at + 24, &node->cx) ||
                !word(scene->bytes + at + 32, &node->cy) || node->verb > (cubic ? 4UL : 3UL) || node->x > 1000 || node->y > 600 ||
                node->cx > 1000 || node->cy > 600 || (j == 0 && node->verb != 0) ||
                (j > 0 && node->verb == 0 && scene->nodes[total - 2].verb != 3 &&
                 !(scene->tool >= 6 && i + 1 == shapes)) ||
                (j > 0 && node->verb != 0 && scene->nodes[total - 2].verb == 3) ||
                (node->verb != 2 && node->verb != 4 && (node->cx || node->cy)) || (node->verb == 3 && (node->x || node->y)))
            {
                return 0;
            }
            node->c2x = node->c2y = 0;
            if (cubic && (!word(scene->bytes + at + 40, &node->c2x) ||
                !word(scene->bytes + at + 48, &node->c2y) || node->c2x > 1000 || node->c2y > 600 ||
                (node->verb != 4 && (node->c2x || node->c2y))))
            {
                return 0;
            }
            at += stride;
        }
    }
    if (at != scene->used)
    {
        return 0;
    }
    if (local)
    {
        const DVM_VectorLocalPlan *plan = &scene->local;
        if (shapes < 2 || plan->source >= shapes-1 || scene->shapes[plan->source].count != plan->count ||
            scene->shapes[shapes-1].count != plan->overlay ||
            !dvm_vector_local_valid(plan,scene->nodes+scene->shapes[plan->source].first,
                scene->nodes+scene->shapes[shapes-1].first))
        {
            return 0;
        }
        scene->local.valid = armed ? 2 : 1;
    }
    scene->count = (unsigned int)shapes;
    scene->complete = 1;
    vector_local_reconcile((unsigned long)(scene-vectors));
    return 1;
}

/* Uniform logical canvas scale; page stays inside the pane beneath its toolbar.
 * Nonpositive means the current very small pane cannot display a drawing. */
static double vector_base(const DeskItem *item)
{
    if (item->width <= 80 || item->height <= 34)
    {
        return 0;
    }
    return fmin((item->width - 80) / 1040.0, (item->height - 34) / 640.0);
}


/* Apply copied owner camera state to page units; no document state is mutated. */
static double vector_scale(const DeskItem *item)
{
    return vector_base(item) * vectors[item->id].zoom / 100.0;
}


/* Project one page coordinate around the independently copied viewport centre. */
static double vector_x(const DeskItem *item, double x)
{
    return 40 + vector_base(item) * 520 + vector_scale(item) * (x - vectors[item->id].center_x);
}


/* Vertical companion to vector_x, in the same containing pane's local pixels. */
static double vector_y(const DeskItem *item, double y)
{
    return 28 + vector_base(item) * 320 + vector_scale(item) * (y - vectors[item->id].center_y);
}


/* Intersect the current native scissor with the fixed canvas viewport. The
 * caller restores all four copied bounds before returning on this owner. */
static void vector_clip(const DeskItem *item, const FVM_Affine *world, int saved[4])
{
    double base = vector_base(item);
    double left = 1e30;
    double top = 1e30;
    double right = -1e30;
    double bottom = -1e30;
    unsigned int corner;
    saved[0] = capture_left;
    saved[1] = capture_top;
    saved[2] = capture_right;
    saved[3] = capture_bottom;
    for (corner = 0; corner < 4; ++corner)
    {
        double x = 40 + base * (corner & 1 ? 1020 : 20);
        double y = 28 + base * (corner & 2 ? 620 : 20);
        double px = world->xx * x + world->xy * y + world->x0;
        double py = world->yx * x + world->yy * y + world->y0;
        left = fmin(left, px);
        top = fmin(top, py);
        right = fmax(right, px);
        bottom = fmax(bottom, py);
    }
    capture_left = (int)fmax(capture_left, floor(left));
    capture_top = (int)fmax(capture_top, floor(top));
    capture_right = (int)fmin(capture_right, ceil(right));
    capture_bottom = (int)fmin(capture_bottom, ceil(bottom));
}


/* Retire one borrowed scissor scope, including empty/failed native drawing. */
static void vector_unclip(const int saved[4])
{
    capture_left = saved[0];
    capture_top = saved[1];
    capture_right = saved[2];
    capture_bottom = saved[3];
}


/* Adapt native stamp/outline preview placement to the copied page camera.
 * Borrow all values; the renderer receives detached geometry through emit. */
static int vector_preview(unsigned long widget, const DeskItem *item, const FVM_Affine *world,
                          DVM_InkEmit emit, void *user)
{
    FVM_Affine camera = *world;
    double dx = vector_x(item, 0) - 40 - 20 * vector_scale(item);
    double dy = vector_y(item, 0) - 28 - 20 * vector_scale(item);
    camera.x0 += world->xx * dx + world->xy * dy;
    camera.y0 += world->yx * dx + world->yy * dy;
    int saved[4];
    int ok;
    vector_clip(item, world, saved);
    ok = dvm_ink_preview(widget, &camera, vector_scale(item), emit, user);
    vector_unclip(saved);
    return ok;
}

#include "../../desktop_node_icons.h"
