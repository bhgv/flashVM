implement DeferredStep;

# Use the Coco/R Limbo big/table profile, shared by the CLI and serial C99 API.
DeferredStep: module
{
    value: fn(): big; # Both DisVM modes and the reference runner return 42.
};


# A counted-for step runs after the body, including a continue. Its operands may
# receive their first values in that body, provided every path to the step writes
# them. Parsing the header alone must never initialize the first body iteration.
value(): big
{
    part: table of big;
    description: string;
    total := big 0;

    # Neither part nor description has a readable value at loop entry. Lowering
    # reserves mapped private nil cells before the test. The step's loads are
    # staged until all incoming body paths have proved their initialization.
    for (index := int 0; index < int 3; total += part.value + big description) {
        index++;
        if (index == int 2) {
            part = table of big {value=7};
            description = "7";
            # This edge contributes facts independently of ordinary fallthrough.
            continue;
        }
        part = table of big {value=7};
        description = "7";
        # Ordinary fallthrough also initializes both operands of the step.
    }

    # The three executed steps each add 7+7. A potentially empty loop still does
    # not prove part/description initialized after exit: read only total here.
    # The native retaining stores replace owners safely; frame teardown releases
    # all roots. No private frame address crosses a channel or becomes a proxy.
    return total;
}
