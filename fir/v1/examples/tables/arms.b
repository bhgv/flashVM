implement LexicalArms;

# The Coco/R compiler recognizes this bounded tutorial entry interface.
# It is compiled directly to FIR and Dis; no external Limbo compiler is used.
LexicalArms: module
{
    value: fn(): big;
};


# Show terminal lexical scopes and the isolation of sibling SSA environments.
# There are no arguments. The selected arm returns the exact big value 42.
# Local tables belong to the executing VM and are retired by its GC; no pointer
# or table representation crosses a channel or native module boundary.
value(): big
{
    # Reading a mutable table field keeps this condition live through optimization.
    # Change the flag to zero to execute the other arm; it also returns 42.
    document := table of big;
    document.flag = big 1;
    baseline := big 40;

    if (document.flag == 1) {
        # The initializer reads the incoming baseline before creating a local
        # shadow. The shadow's SSA value and lexical identity belong to this arm.
        baseline := baseline + 2;

        # Typed declarations may be initialized later within the same arm.
        answer: big;
        answer = baseline;

        # Local owners and aliases are allowed before the terminal return.
        # Mutations are ordinary selected-arm effects, not remote atomic updates.
        result := table of big;
        alias := result;
        alias.answer = answer;

        # Reassigning the outer owner is valid here. It must not make the parser
        # use this arm's result table while compiling the sibling below.
        document = result;
        return document.answer;
    } else {
        # This is a new lexical scope. baseline is the incoming big 40, and
        # document is the incoming table, regardless of the first arm's writes.
        # Names declared in the first arm (answer/result/alias) are unavailable.
        answer := baseline + 2;
        return answer;
    }
}
