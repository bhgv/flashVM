/* Private desktop integration include: value plans bind native motion to paths. */


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
    if (scene->local.valid && dvm_vector_local_matches(local_edit,&scene->local,
        scene->nodes+scene->shapes[scene->local.source].first,
        scene->nodes+scene->shapes[scene->count-1].first))
    {
        long x;
        long y;
        dvm_vector_local_point(local_edit,&x,&y);
        dvm_vector_local_move(local_edit,x,y,scene->nodes+scene->shapes[scene->local.source].first,
            scene->nodes+scene->shapes[scene->count-1].first);
    }
    else
    {
        dvm_vector_local_destroy(local_edit);
        local_edit = NULL;
        local_widget = 0;
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
    if (!point || widget < 1 || widget > 8 || pending || !vectors[widget].complete ||
        !vectors[widget].local.valid || *point < 0 || *point > 601600 ||
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
