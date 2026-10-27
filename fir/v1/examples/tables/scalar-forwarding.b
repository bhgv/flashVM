implement ScalarForwarding;

# Select the Coco/R Limbo big/table profile shared by CLI and C99 IDE clients.
ScalarForwarding: module
{
    value: fn(): big; # Original and optimized programs both return 42.
};


# Several reads and writes of the same private accumulator occur within one
# basic block. FIR can reuse the most recently stored scalar SSA value instead
# of loading the private cell again. This does not change assignment order.
value(): big
{
    total := big 0;
    for (index := int 0; index < int 3; index++) {
        part := big 7;
        total = total + part;

        # Both reads see the preceding store. They become copies of its payload,
        # then ordinary copy propagation removes the redundant load results.
        doubled := total + total;

        # This read still sees that same stored value. The new final store must
        # remain: subsequent loop iterations require the updated accumulator.
        total = doubled - total + part;
    }

    # Each iteration adds 14. The exit is a separate block, so local forwarding
    # does not guess a value across the loop's backedge or its zero-trip path.
    # Table elements, retained reference loads and external channel projections
    # are outside this optimization; ownership publication is not removed.
    return total;
}
