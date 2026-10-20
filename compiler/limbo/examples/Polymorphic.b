# Syntax-only example; no includes, Dis instructions or channel execution are generated.
implement Polymorphic; # The implementation name is syntactically declared here.
Polymorphic: module { # Interface declarations share the same parameter syntax.
    identity: fn[T](value: T): T; # A polymorphic function type preserves its type variable.
}; # Finish the interface, including its mandatory semicolon.
failure: exception(string); # A typed exception declaration is distinct from a handler.
identity[T](value: T): T { # The definition declares its own polymorphic parameter.
    return value; # Return and type checking remain separate from syntax recognition.
} # Finish the function definition.
handle() raises failure { # An explicit raises list can also use parentheses for several names.
    { # Begin the protected statement block.
        raise failure("example"); # Construct and raise the declared exception syntactically.
    } exception error { # Bind the caught exception for handler code.
        failure => raise; # A bare raise rethrows; context checking is a later pass.
        * => exit; # Wildcard handler has explicit spacing before its arrow.
    } # Finish the handler selection.
} # Finish the function definition.
