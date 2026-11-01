/* Shared owner-local positions for vector MovieClip controls and native hit tests. */
#ifndef DVM_EDITOR_VECTOR_LAYOUT_H
#define DVM_EDITOR_VECTOR_LAYOUT_H


/* Return one copied control rectangle. Indices retain the existing channel
 * command mapping;16 is Finish. Width is the containing logical pane extent.
 * No VM or document is accessed. Negative/out-of-range indices are refused. */
static int dvm_vector_control_rect(unsigned int index, double width, double *x, double *y)
{
    static const unsigned int rows[19] = {2, 3, 4, 5, 7, 8, 9, 10, 0, 1, 2, 3, 4, 5, 11, 6, 12, 0, 1};
    if (index >= 19 || width < 80)
    {
        return 0;
    }
    *x = index >= 8 && index < 14 ? width - 34 : 6;
    *y = 28 + rows[index] * 28;
    return 1;
}
#endif
