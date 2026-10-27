implement LoopControl;

# CLI and the C99 compiler API share the authoritative Coco/R grammar.
LoopControl: module
{
    value: fn(): big; # No arguments; returns the checked big result 42.
};


# Filter a heterogeneous-key table and stop when the required sum is reached.
# Table/key owners remain local GC roots. Each assignment keeps its own lease;
# branch and loop transfers never jump out of a partly emitted assignment.
value(): big
{
    entries := table of big {10, 20, 22, 100, note=999};
    sum := big 0;

    # Capture keys once. continue must advance this private snapshot cursor,
    # while break skips cursor advancement and exits the innermost traversal.
    for (key, value in entries) {
        if (typeof key == "string") {
            continue;
        } else if (value == 10) {
            # This iteration is intentionally filtered out.
            continue;
        }

        # Body-local big values can be changed in either arm and used afterward.
        selected := big 0;
        if (value < 30) {
            selected = value;
        } else {
            selected = 0;
        }
        sum += selected;

        if (sum == 42) {
            break;
        }
    }

    # while-continue returns to the test rather than a for-in cursor step.
    # This independent loop reaches 3 and confirms nested transfer state retired.
    count := big 0;
    while (count < 3) {
        count += 1;
        if (count < 3) {
            continue;
        } else {
            break;
        }
    }

    # Keep the existing two-trace tutorial harness for installed SDK clients.
    if (sum == 42 && count == 3) {
        return sum;
    } else {
        return 0;
    }
}
