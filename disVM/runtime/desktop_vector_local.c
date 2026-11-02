/* Private desktop integration include: value plans bind native motion to paths. */


/* Select a rendered marker from copied owner-issued bindings and start its
 * detached baseline immediately. Native hit admission is a conservative subset
 * of the owner's hit radius: only the visible marker circle, no contour query.
 * Preserve an already selected group; otherwise select only this explicit point. */
int dvm_desktop_local_begin(unsigned long widget, long point)
{
    VectorScene *scene;
    DVM_VectorLocalPlan *plan;
    DVM_VectorCommand *source;
    unsigned long i;
    unsigned long hit = 0;
    long best = 0x7fffffffL;
    long x = point%1001;
    long y = point/1001;
    if (widget < 1 || widget > 8 || point < 0 || point > 601600 ||
        pending || local_edit || !vectors[widget].complete || vectors[widget].local.valid != 2)
    {
        return 0;
    }
    scene = &vectors[widget];
    source = scene->nodes+scene->shapes[scene->local.source].first;
    for (i = scene->local.count; i < scene->local.overlay; ++i)
    {
        unsigned long binding = scene->local.bindings[i];
        unsigned long flags = binding&255;
        unsigned long code = binding>>8;
        if ((flags&7) == 1 && code)
        {
            DVM_VectorCommand *n = source+(code-1)/3;
            unsigned int p = (unsigned int)((code-1)%3);
            long px = (long)(p == 0 ? n->x : p == 1 ? n->cx : n->c2x);
            long py = (long)(p == 0 ? n->y : p == 1 ? n->cy : n->c2y);
            long distance = (px-x)*(px-x)+(py-y)*(py-y);
            long radius = (long)(flags>>3);
            if (distance <= radius*radius && distance <= best)
            {
                best = distance;
                hit = code;
            }
        }
    }
    if (!hit || !(plan = malloc(sizeof(*plan))))
    {
        return 0;
    }
    *plan = scene->local;
    if (!(plan->masks[(hit-1)/3] & (1UL<<((hit-1)%3))))
    {
        for (i = 0; i < plan->count; ++i)
        {
            plan->masks[i] &= ~7UL;
        }
    }
    plan->masks[(hit-1)/3] |= 1UL<<((hit-1)%3);
    plan->x = (unsigned long)x;
    plan->y = (unsigned long)y;
    plan->valid = 1;
    local_edit = dvm_vector_local_create(plan,source,scene->nodes+scene->shapes[scene->count-1].first);
    free(plan);
    if (!local_edit)
    {
        return 0;
    }
    local_widget = widget;
    local_released = 0;
    return 1;
}


/* Permit only captured motion on the previous fully decoded scene. */
int dvm_desktop_local_ready(void)
{
    return local_edit && !local_released && local_widget >= 1 && local_widget <= 8 &&
        !pending && vectors[local_widget].complete;
}


/* Discard a baseline only when its matching canonical plan disappears/changes;
 * unrelated owner snapshots retain and reapply the latest local coordinates. */
static void vector_local_reconcile(unsigned long widget)
{
    VectorScene *scene;
    if (!local_edit || widget != local_widget || widget > 8)
    {
        return;
    }
    scene = &vectors[widget];
    if (local_released && scene->local.valid == 2)
    {
        dvm_vector_local_destroy(local_edit);
        local_edit = NULL;
        local_widget = 0;
        local_released = 0;
        return;
    }
    if ((scene->local.valid && dvm_vector_local_matches(local_edit,&scene->local,
        scene->nodes+scene->shapes[scene->local.source].first,
        scene->nodes+scene->shapes[scene->count-1].first)) ||
        (!local_released && dvm_vector_local_idle_matches(local_edit,&scene->local,
            scene->nodes+scene->shapes[scene->local.source].first)))
    {
        long x;
        long y;
        dvm_vector_local_point(local_edit,&x,&y);
        dvm_vector_local_move(local_edit,x,y,scene->nodes+scene->shapes[scene->local.source].first,
            scene->nodes+scene->shapes[scene->count-1].first);
    }
    else if (dvm_vector_local_source_matches(local_edit,&scene->local,
        scene->nodes+scene->shapes[scene->local.source].first))
    {
        long x;
        long y;
        DVM_VectorLocal *acknowledged = dvm_vector_local_create(&scene->local,
            scene->nodes+scene->shapes[scene->local.source].first,
            scene->nodes+scene->shapes[scene->count-1].first);
        if (acknowledged)
        {
            dvm_vector_local_point(local_edit,&x,&y);
            dvm_vector_local_destroy(local_edit);
            local_edit = acknowledged;
            dvm_vector_local_move(local_edit,x,y,scene->nodes+scene->shapes[scene->local.source].first,
                scene->nodes+scene->shapes[scene->count-1].first);
            ++local_epochs[widget];
        }
        else
        {
            dvm_vector_local_destroy(local_edit);
            local_edit = NULL;
            local_widget = 0;
            local_released = 0;
        }
    }
    else
    {
        dvm_vector_local_destroy(local_edit);
        local_edit = NULL;
        local_widget = 0;
        local_released = 0;
    }
}


/* Translate selected source points and bound decorations without a per-motion
 * Dis request. The modifier plan and selection authority came from its owner. */
int dvm_desktop_local_motion(unsigned long widget, long *point)
{
    VectorScene *scene;
    long x;
    long y;
    long previous_x;
    long previous_y;
    int changed;
    if (!point || local_released || widget < 1 || widget > 8 || pending || !vectors[widget].complete ||
        (!vectors[widget].local.valid || (!local_edit && vectors[widget].local.valid != 1)) || *point < 0 || *point > 601600 ||
        (local_edit && local_widget != widget))
    {
        return 0;
    }
    scene = &vectors[widget];
    if (!local_edit)
    {
        local_edit = dvm_vector_local_create(&scene->local,
            scene->nodes+scene->shapes[scene->local.source].first,
            scene->nodes+scene->shapes[scene->count-1].first);
        if (!local_edit)
        {
            return 0;
        }
        local_widget = widget;
    }
    x = *point%1001;
    y = *point/1001;
    dvm_vector_local_point(local_edit,&previous_x,&previous_y);
    changed = dvm_vector_local_move(local_edit,x,y,
        scene->nodes+scene->shapes[scene->local.source].first,
        scene->nodes+scene->shapes[scene->count-1].first);
    dvm_vector_local_point(local_edit,&x,&y);
    *point = y*1001+x;
    if (changed && (x != previous_x || y != previous_y))
    {
        ++local_epochs[widget];
        return 2;
    }
    return 1;
}


/* Inspect exclusively owned local gesture state without a VM or channel call. */
int dvm_desktop_local_active(void)
{
    return local_edit != NULL;
}


/* Restore the baseline before a copied cancellation reaches the document owner. */
void dvm_desktop_local_cancel(void)
{
    if (local_edit && local_widget >= 1 && local_widget <= 8 && vectors[local_widget].local.valid)
    {
        VectorScene *scene = &vectors[local_widget];
        dvm_vector_local_restore(local_edit,scene->nodes+scene->shapes[scene->local.source].first,
            scene->nodes+scene->shapes[scene->count-1].first);
        ++local_epochs[local_widget];
    }
    dvm_vector_local_destroy(local_edit);
    local_edit = NULL;
    local_widget = 0;
    local_released = 0;
}


/* Produce a scalar release proposal before a keyboard/lifecycle barrier. */
int dvm_desktop_local_position(unsigned long *widget, long *point)
{
    long x;
    long y;
    if (!local_edit || !widget || !point)
    {
        return 0;
    }
    dvm_vector_local_point(local_edit,&x,&y);
    *widget = local_widget;
    *point = y*1001+x;
    return 1;
}


/* Retain the last visual state until canonical idle/changed geometry arrives. */
void dvm_desktop_local_release(void)
{
    if (local_edit)
    {
        local_released = 1;
    }
}
