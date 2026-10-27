implement ScalarStores;

# Select the Coco/R Limbo big/table profile used by CLI and C99 IDE clients.
ScalarStores: module
{
    value: fn(): big; # Original and optimized execution return 42.
};


# Intermediate private scalar writes need not reach the frame when a later write
# replaces them in the same block before observation. Their RHS values remain
# available as SSA payloads; independently effectful computations must still run.
value(): big
{
    total := big 0;
    for (index := int 0; index < int 3; index++) {
        # Each following read can use the preceding stored SSA payload directly.
        # The optimizer then drops the overwritten first and second stores.
        total = total + 1;
        saved := total;
        total = total + 1;
        total = total + 12;

        # saved captures a value, not the cell itself. It stays equal to the
        # first intermediate total even after subsequent stores. This pure
        # expression does not publish the cell or introduce a barrier.
        difference := total - saved;
        total = saved + difference;
    }

    # The last store in every body is retained for the next iteration and exit.
    # Three iterations each add 14; no facts cross the loop's backedge.
    # Reference stores and channel projections are outside this optimization.
    return total;
}
