implement Idetoolpalette;
include "idetoolpalette.m";
include "idetools.m";


# Select a bounded grid for copied canvas height. No presentation/owner is stored.
columns(height: int): int
{
    if (height < 340) {
        return 4;
    }
    return 3;
}


# Reserve four unwrapped recent-path labels without crowding tiny canvases.
# Return copied pixels; no owner state or blocking calls.
legend(height: int): int
{
    if (height < 300) {
        return 0;
    }
    return 72;
}


# Anchor a rounded body to the invoking button. notes reserves a readable four-
# line recent-path legend. Return a canvas-clamped body, leaving 12px for its tip.
bounds(owner_y, count, notes, width, height: int): (int, int, int, int)
{
    cols := columns(height);
    rows := (count + cols - 1) / cols;
    occupied := cols;
    if (count < occupied) {
        occupied = count;
    }
    wide := occupied * (Idetools->ButtonWidth + 5) + 11;
    high := rows * Idetools->ButtonStep + 11 + notes * legend(height);
    x := width - Idetools->RightWidth + Idetools->ButtonLeft - 4 - 12 - wide;
    y := owner_y - 8;
    if (y + high > height - 8) {
        y = height - 8 - high;
    }
    if (y < 48) {
        y = 48;
    }
    return (x, y, wide, high);
}


# Map one admitted command row to a common-sized button. Inputs/outputs are
# copied pixels. The caller controls row bounds and source command identity.
cell(row, x, y, height: int): (int, int)
{
    cols := columns(height);
    return (x + 8 + (row % cols) * (Idetools->ButtonWidth + 5),
            y + 8 + (row / cols) * Idetools->ButtonStep);
}


# Invert the exact cell geometry; refuse borders, gaps, footer and empty cells.
# No command executes here. Return -1 for every nonbutton coordinate.
hit(px, py, x, y, count, height: int): int
{
    cols := columns(height);
    dx := px - x - 8;
    dy := py - y - 8;
    if (dx < 0 || dy < 0 || dx / (Idetools->ButtonWidth + 5) >= cols ||
        dx % (Idetools->ButtonWidth + 5) >= Idetools->ButtonWidth ||
        dy % Idetools->ButtonStep >= Idetools->ButtonHeight) {
        return -1;
    }
    row := (dy / Idetools->ButtonStep) * cols + dx / (Idetools->ButtonWidth + 5);
    if (row >= count) {
        return -1;
    }
    return row;
}


# Test copied body bounds; the fixed right shelf separately owns its small tip.
contains(px, py, x, y, wide, high: int): int
{
    return px >= x && px < x + wide && py >= y && py < y + high;
}
