# Syntax-only example: no Dis code generation or type resolution.
implement Syntax;
run(input: array of int, ready: chan of int) {
    values := list of {1, 2, 3};
    weights := array[8] of {0 => 1, * => 0,};
    tail := input[1:];
    outer: for(i := 0; i < len input; i++) {
        if(input[i] == 0) continue outer;
        break outer;
    }
    selection: alt {
        value := <-ready =>
            case value { 0 => break selection; * => exit; }
        * => break selection;
    }
}
