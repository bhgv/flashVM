implement Idethumbnails;
include "idethumbnails.m";


# Keep readable portrait cards; overflowing owners are reached by shelf scrolling.
# count is the number of live owners, including minimized ones. No mutable state.
step(count, height: int): int
{
    return 136;
}


# Return the backing array ordinal for a live visible card. present is indexed by
# document identity-1; every array belongs to the caller. Invalid proposals return-1.
slot(order, present: array of int, rank: int): int
{
    if (rank < 0) {
        return -1;
    }
    for (index := 0; index < len order; index++) {
        identity := order[index];
        if (identity < 0 || identity >= len present || !present[identity]) {
            continue;
        }
        if (rank == 0) {
            return index;
        }
        rank--;
    }
    return -1;
}
