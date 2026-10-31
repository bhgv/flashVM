implement Idedocking;
include "idedocking.m";


# x/y are copied canvas pixels of the active dragging contact. The top bar is
# outside the shelf. No window-edge position, frame clock or UI pointer is used.
inside(x, y: int): int
{
    return x >= 0 && x < 80 && y >= 48;
}
