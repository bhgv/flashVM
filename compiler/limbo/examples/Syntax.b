# Syntax-only tutorial: no Dis/FIR backend, type resolution or channel runtime here.
implement Syntax;


run(input: array of int, ready: chan of int)
{
    values := list of
    {
        1, 2, 3
    };  # Nonempty list literal; an empty list uses nil.
    weights := array[8] of
    {
        0 => 1, * => 0,
    };                  # Explicit size, wildcard and trailing comma.
    tail := input[1:];  # The lower bound is required; the upper bound is optional.

    # A loop label is distinguished from an ordinary typed declaration.
    outer: for(i := 0; i < len input; i++)
    {
        if(input[i] == 0)
        {
            continue outer;  # The syntax frontend does not validate target-label scope.
        }
        break outer;
    }

    # A labeled alt uses arrow lookahead to find the next outer qualifier.
    selection: alt
    {
        value := <-ready =>
        case value
        {
            0 => break selection;
            * => exit;  # Write * => with a space: *=> starts with the *= token.
        }
        * => break selection;
    }
}
