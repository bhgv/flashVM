implement LazyConditions;

# This interface selects our exact big/table Coco/R frontend.
# Both the CLI and the C99 source API translate it directly to FIR and Dis.
LazyConditions: module
{
    value: fn(): big;
};


# Return 42 while demonstrating skipped reads, skipped mutations and lazy joins.
# No arguments. The document owner stays inside this VM; only the big result is
# exported. No remote atomicity or shared-variable transaction is implied.
value(): big
{
    document := table of big;
    document.answer = big 42;

    # If the key is absent, && skips the read, so an absent-key error cannot occur.
    # Both operands have static bool type; numbers are never implicitly truthy.
    present := "answer" in document;
    valid := present && (document.answer == 42);

    # valid is true here. Therefore || skips delete and preserves answer.
    # The compiler emits the RHS only inside its own guarded control-flow arm.
    selected := valid || (delete document.answer);

    # A second lazy expression feeds an ordinary terminal source decision.
    # Each logical result is normalized bool, independent of the hidden merge cell.
    if (selected && ("answer" in document)) {
        # The preserved table entry proves that the skipped delete did not run.
        return document.answer;
    } else {
        # This unselected arm would fail if accidentally executed.
        return 1 / 0;
    }
}
