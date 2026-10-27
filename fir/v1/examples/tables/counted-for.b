implement CountedFor;

# The CLI and in-process C99 compiler API use the same Coco/R grammar.
CountedFor: module
{
    value: fn(): big; # No arguments; exact signed-big result is 42.
};


# Count, filter and mutate table entries with a classic three-clause for.
# Initializer/body names share a loop-local scope. Table writes keep their own
# recursive assignment lease; neither break nor return executes the header step.
value(): big
{
    values := table of big;
    sum := big 0;

    # The initializer runs once. The condition runs before every pass. The step
    # is parsed in this header but emitted after the body, on its own CFG block.
    for (index := big 0; index < 100; index++) {
        if (index % 2 != 0) {
            # Continue advances index once, then reevaluates the condition.
            continue;
        }
        if (index == 14) {
            # Break bypasses index++ and leaves the loop immediately.
            break;
        }
        values[index] = index;
        sum += values[index];
    }

    # Standalone postfix updates also work on captured table locations. The key
    # and receiver are evaluated once, under the same lease as read/modify/write.
    values.answer = big 40;
    values.answer++;
    values.answer++;
    values.answer--;

    # Count down with an ordinary compound step and an empty initializer.
    check := big 3;
    for (; check > 0; check -= 1) {
        values.answer++;
    }

    # sum is 0+2+4+6+8+10+12 = 42; answer is now 44. Snapshot for-in remains
    # available alongside this syntax and is documented in its own tutorial.
    if (sum == 42 && values.answer == 44 && check == 0) {
        return sum;
    } else {
        return 0;
    }
}
