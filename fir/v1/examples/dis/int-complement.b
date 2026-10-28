implement IntComplement; # Select the Limbo module with a typed table body.
IntComplement: module { # Export a scalar capture without sharing the table owner.
    value: fn(): big; # The typed table profile returns an exact signed64 value.
}; # All temporary storage remains inside the function below.
value(): big { # The hosted Dis backend owns and later collects this local table.
    Number: type int; # Keep the operand signed32 even though the table stores big.
    values := table of big {answer = big -43}; # Store an input that requires a runtime load.
    input: Number = Number values.answer; # Explicitly narrow the loaded big to signed32.
    answer := ~input; # Lower to i32.xor with minus one, evaluating input just once.
    return big answer; # Widen only the final exported result; the captured value is 42.
} # GC must release the temporary table in both interpreter and native dispatch.
