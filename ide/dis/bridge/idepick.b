implement Idepick;
include "idedesktop.m";
include "idepick.m";
include "idetools.m";


# Walk the exact copied paint order backwards. Parent transforms are bounded to
# the table or one MDI root. Inputs are borrowed immutable descriptors and copied
# canvas geometry. Return0 for no hit, including hidden offscreen descriptors.
topmost(items: array of ref Idedesktop->Item, x, y, width, zoom, pan, scroll: int): int
{
    for (index := len items - 1; index >= 0; index--) {
        item := items[index];
        left := item.x;
        top := item.y;
        wide := item.width;
        high := item.height;
        parent := item.parent;
        if (parent >= 5 && parent <= 12) {
            identity := parent + 8;
            if (parent == 12) {
                identity = 28;
            }
            found := 0;
            for (root := 0; root < len items; root++) {
                if (items[root].id == identity) {
                    left += items[root].x;
                    top += items[root].y;
                    parent = items[root].parent;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                continue;
            }
        }
        if (parent == 1) {
            left = 84 + (left - 84) * zoom / 100 + pan;
            top = 48 + (top - 48) * zoom / 100 - scroll;
            wide = wide * zoom / 100;
            high = high * zoom / 100;
        } else if (parent == 3) {
            left += width - Idetools->RightWidth;
        }
        if (x >= left && x < left + wide && y >= top && y < top + high) {
            return item.id;
        }
    }
    return 0;
}
