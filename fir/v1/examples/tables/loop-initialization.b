implement LoopInitialization;

# Select the big/table compiler profile. The same source is accepted by the CLI
# and the serial C99 compiler service used by an IDE.
LoopInitialization: module
{
    value: fn(): big; # The reference runner and both DisVM modes return 42.
};


# Reuse outer declarations as iteration scratch values without inventing an
# initial string or table. A first write makes later reads in this body valid;
# the next iteration and the continuation still need their own source proof.
value(): big
{
    text: string;
    item: table of big;
    amount: int;
    total := big 0;

    # The compiler creates private typed cells before the loop test. String and
    # table cells initially hold canonical nil, not usable source values. Each
    # iteration stores real owners before reading them. Replacing a table owner
    # uses retaining MOVP in Dis; the final frame teardown releases its roots.
    for (index := int 0; index < int 3; index++) {
        if (index == int 0) {
            text = "14";
        } else {
            text = "14";
        }
        amount = int text;
        item = table of big {value=big amount};
        total += item.value;
    }

    # Even though this particular loop ran three times, the compiler does not
    # prove trip counts. Reading text/item/amount here without new writes would
    # be rejected. This explicit write establishes a fresh continuing-path fact.
    text = string total;

    # A nested loop may read the freshly assigned text, including on its first
    # iteration. The empty body does not export any new initialization facts.
    for (index := int 0; index < int 0; index++) {
        amount = int text;
    }

    # Do not initialize an unreadable outer local in a counted-for header step:
    # the step executes after the body and cannot establish its initial facts.
    # Use body assignments as above, or initialize before the whole loop.
    return big text;
}
