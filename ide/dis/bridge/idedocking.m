# Pointer-driven left-shelf policy, independent of window bounds and rendering.
Idedocking: module {
    PATH: con "idedocking.dis"; # Matching pure gesture policy.
    # Return true when the dragging contact is inside the visible left shelf.
    inside: fn(x, y: int): int;
};
