# Pure left-shelf card geometry and stable-order mapping; no document ownership.
Idethumbnails: module {
    PATH: con "idethumbnails.dis"; # Matching portable card layout.
    # Return vertical spacing, adapting to live owner count and canvas pixels.
    step: fn(count, height: int): int;
    # Map a visible ordinal to an index in a borrowed stable order, or-1.
    slot: fn(order, present: array of int, rank: int): int;
};
