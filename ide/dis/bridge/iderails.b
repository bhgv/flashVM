implement Iderails;
include "iderails.m";


# Clamp an independent rail translation. count/spacing/top describe its content;
# height is the viewport. Return pixels without mutating descriptors or owners.
clamp(offset, count, spacing, top, height: int): int
{
    maximum := top + count * spacing + 8 - height;
    if (maximum < 0) {
        maximum = 0;
    }
    if (offset < 0) {
        offset = 0;
    }
    if (offset > maximum) {
        offset = maximum;
    }
    return offset;
}


# Classify a physical pointer without acquiring any document or native handle.
# The fixed top bar owns y<48. Return zero for desktop and outside coordinates.
region(x, y, width: int): int
{
    if (y < 48 || x < 0 || x >= width) {
        return 0;
    }
    if (x < 80) {
        return 2;
    }
    if (x >= width - 104) {
        return 3;
    }
    return 0;
}
