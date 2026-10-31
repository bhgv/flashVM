implement Idetabview;
include "idetabview.m";


# Return the content region below header/tabs, preserving a small outer card rim.
# Inputs/outputs are copied logical pixels; this function never changes an owner.
body(x, y, wide, high: int): (int, int, int, int)
{
    return (x + 8, y + 60, wide - 16, high - 68);
}


# Divide the scaled strip into count equal cells. Caller supplies count>0.
width(wide, count, zoom: int): int
{
    return (wide - 16 * zoom / 100) / count;
}


# Use the same integer geometry for painting and admission; no native hit test
# is trusted as document authority. Return0/1, borrowing no reference values.
hit(px, py, x, y, wide, count, rank, zoom: int): int
{
    cell := width(wide, count, zoom);
    left := x + 8 * zoom / 100 + rank * cell;
    return px >= left && px < left + cell && py >= y + 28 * zoom / 100 && py < y + 54 * zoom / 100;
}
