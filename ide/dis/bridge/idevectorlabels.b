implement Idevectorlabels;
include "idevector.m";
include "idevectorlabels.m";


# Format checked current tool state for the owner and desktop caption. Scalars
# use Idevector's existing vocabulary; invalid values return an empty label.
# Returned strings are caller-owned immutable values; this pure call never blocks.
state(tool, width, fill: int): string
{
    if (tool < Idevector->Brush || tool > Idevector->Pen || width < 1 || width > 32 ||
        (fill != 0 && fill != 1)) {
        return "";
    }
    names := array[] of {"", "Brush", "Rectangle", "Ellipse", "Line", "Pen"};
    value := "off";
    if (fill) {
        value = "on";
    }
    label := "Fill:";
    if (tool == Idevector->Brush || tool == Idevector->Pen || tool == Idevector->Line) {
        label = "Shape fill:";
    }
    size := "Width:" + string width;
    if (tool == Idevector->Brush) {
        size = "Brush size:" + string width;
    } else if (tool != Idevector->Pen) {
        size = "Stroke:1 | Brush size:" + string width;
    }
    return names[tool] + " | " + label + value + " | " + size;
}
