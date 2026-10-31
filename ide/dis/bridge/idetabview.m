# Pure tab-view geometry; no document state, rendering or channel ownership.
Idetabview: module {
    PATH: con "idetabview.dis"; # Matching portable implementation.
    # Return content rectangle in the parent's logical coordinate system.
    body: fn(x, y, wide, high: int): (int, int, int, int);
    # Return a tab width for a positive count and percent zoom; caller validates count.
    width: fn(wide, count, zoom: int): int;
    # Test a copied canvas point against one tab in an already projected parent.
    hit: fn(px, py, x, y, wide, count, rank, zoom: int): int;
};
