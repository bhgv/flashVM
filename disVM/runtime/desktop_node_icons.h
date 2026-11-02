/* Small original node-editing glyphs, rendered with generic MovieClip strokes. */
#ifndef DVM_DESKTOP_NODE_ICONS_H
#define DVM_DESKTOP_NODE_ICONS_H


/* Draw one supplied operation glyph,0..8, into a caller-owned MovieClip.
 * context/clip are owner-thread only; extent and RGB affect presentation only.
 * Each independent segment begins with moveTo. Return0 on invalid index or VM
 * failure; no document access, event handling or ownership transfer occurs. */
static int dvm_node_icon(FVM_Context *context, FVM_Object *clip, unsigned int index,
                          double extent, unsigned long color)
{
    static const unsigned int counts[9] = {6,5,6,8,5,7,6,7,7}; /* Initialized segments. */
    static const signed char lines[9][8][4] = {
        {{-8,0,8,0},{0,-7,0,7},{-3,-3,3,-3},{3,-3,3,3},{3,3,-3,3},{-3,3,-3,-3}},
        {{-8,0,8,0},{-3,-3,3,-3},{3,-3,3,3},{3,3,-3,3},{-3,3,-3,-3}},
        {{-9,-5,-3,-1},{3,1,9,5},{-6,-4,-2,-4},{-2,-4,-2,0},{2,0,2,4},{2,4,6,4}},
        {{-9,0,-4,0},{4,0,9,0},{-4,-3,-1,-3},{-1,-3,-1,3},{-1,3,-4,3},{1,-3,4,-3},{4,-3,4,3},{4,3,1,3}},
        {{-8,-5,8,5},{-9,-6,-6,-6},{-6,-6,-6,-3},{6,3,9,3},{9,3,9,6}},
        {{-9,5,-5,-3},{-5,-3,0,-5},{0,-5,5,-3},{5,-3,9,5},{-9,5,-9,-6},{-9,-6,9,-6},{9,-6,9,5}},
        {{-9,6,0,-5},{0,-5,9,6},{-3,-5,3,-5},{3,-5,3,1},{3,1,-3,1},{-3,1,-3,-5}},
        {{-9,4,0,0},{0,0,6,-3},{-9,4,-9,7},{6,-3,6,-6},{-2,-2,2,-2},{2,-2,2,2},{2,2,-2,2}},
        {{-8,4,0,0},{0,0,8,-4},{-8,4,-8,7},{8,-4,8,-7},{-2,-2,2,-2},{2,-2,2,2},{2,2,-2,2}}
    }; /* Add/delete/break/join/line/cubic/cusp/smooth/symmetric diagrams. */
    FVM_Value values[3];
    FVM_Value ignored;
    unsigned int i;
    double scale = extent >= 22 ? 1 : extent > 4 ? (extent-4)/18 : 0;
    if (index >= 9)
    {
        return 0;
    }
    fvm_value_number(values,1.5);
    fvm_value_number(values+1,color);
    fvm_value_number(values+2,100);
    if (fvm_object_call(context,clip,"lineStyle",3,values,&ignored) != FVM_OK)
    {
        return 0;
    }
    for (i = 0; i < counts[index]; ++i)
    {
        fvm_value_number(values,extent/2+scale*lines[index][i][0]);
        fvm_value_number(values+1,11+scale*lines[index][i][1]);
        if (fvm_object_call(context,clip,"moveTo",2,values,&ignored) != FVM_OK)
        {
            return 0;
        }
        fvm_value_number(values,extent/2+scale*lines[index][i][2]);
        fvm_value_number(values+1,11+scale*lines[index][i][3]);
        if (fvm_object_call(context,clip,"lineTo",2,values,&ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}
#endif
