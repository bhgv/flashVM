/* Shared owner-local positions for vector MovieClip controls and native hit tests. */
#ifndef DVM_EDITOR_VECTOR_LAYOUT_H
#define DVM_EDITOR_VECTOR_LAYOUT_H


/* Keep the nine-node command row inside a narrow logical pane. Legacy vertical
 * controls retain28-unit widths; node spacing shrinks only with available room. */
static double dvm_vector_control_width(unsigned int index, double width)
{
    double step = (width-80)/9;
    if (index < 23)
    {
        return 28;
    }
    return step >= 30 ? 28 : step > 4 ? step-2 : 2;
}


/* Return one copied control rectangle. Indices retain the existing channel
 * command mapping;16 is Finish. Width is the containing logical pane extent.
 * No VM or document is accessed. Negative/out-of-range indices are refused. */
static int dvm_vector_control_rect(unsigned int index, double width, double *x, double *y)
{
    static const unsigned int rows[20] = {3, 4, 5, 6, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 12, 7, 13, 0, 1, 2};
    if (index >= 32 || width < 80)
    {
        return 0;
    }
    if (index >= 20 && index <= 22)
    {
        return 0;
    }
    if (index >= 23)
    {
        double step = (width-80)/9;
        *x = 42 + (index - 23) * (step < 30 ? step : 30);
        *y = 28;
        return 1;
    }
    *x = index >= 8 && index < 14 ? width - 34 : 6;
    *y = 28 + rows[index] * 28;
    return 1;
}
#endif
