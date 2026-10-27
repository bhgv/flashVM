implement FunctionFlow;

# A big result selects the typed hybrid-table compiler profile.
FunctionFlow: module
{
    value: fn(): big; # The reference runner and both DisVM modes return 42.
};


# Ordinary conditions may update typed locals and then continue the function.
# Early-return paths keep their own lexical environment. Private scalar/reference
# cells carry updates through joins without exporting addresses to another VM.
value(): big
{
    settings := table of big {enabled=1};
    answer := big 40;
    label := "pending";

    # The false path returns immediately. Only the true path reaches the suffix.
    if (settings.enabled == 1) {
        answer += 2;
        label = "ready";
    } else {
        return 0;
    }

    # An independent lexical block may shadow a name without changing its parent.
    {
        label := "inner";
        if (label == "inner") {
            settings.checked = big 1;
        }
    }

    # Optional else and else-if form ordinary statements at function scope.
    if (answer < 0) {
        return 0;
    } else if (answer != 42) {
        return 0;
    }

    # Loop and function conditions share the same scoped statement grammar.
    for (index := int 0; index < int 2; index++) {
        {
            if (index == int 0) {
                continue; # Still targets the for step, through the nested block.
            }
            settings.visits = big index;
        }
    }

    # Both branches terminate, so no unreachable join block is emitted.
    if (label == "ready" && settings.checked == 1 && settings.visits == 1) {
        return answer;
    } else {
        return 0;
    }
}
