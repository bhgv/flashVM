implement Uiecho; # Compile an ordinary Dis module rather than a host-language service.
include "uibridge.m"; # Declare the shared wire codec and copied message type.
include "uiecho.m"; # Declare the host-callable teaching service entry point.


# Process ordinary blocking Dis channels; the scheduler suspends this Limbo process.
# u borrows the loaded codec; input/output remain owned by the calling host.
# Returns zero on acknowledged shutdown or minus one on invalid protocol/operation.
serve(u: Uibridge, input: chan of array of byte, output: chan of array of byte): int
{
    for (;;) { # Remain available until a validated shutdown request arrives.
        frame := <-input; # Receive an owned byte-array reference through the Dis mover.
        (ok, message) := u->decode(frame); # Validate bytes before using any application field.
        if (!ok || message.kind != u->Request) { # Admit correlated requests only.
            return -1; # Let the embedding host report a malformed application request.
        } # Finish request admission.
        operation := message.code; # Save the requested operation before writing reply status.
        if (operation != 1 && operation != 2) { # This example supports echo and shutdown only.
            return -1; # Do not claim success for an unknown command.
        } # Finish operation validation.
        message.kind = u->Reply; # Preserve correlation ID and text in the response record.
        message.code = 0; # Report successful handling using the example's status convention.
        (encoded, response) := u->encode(message); # Allocate an independent outgoing frame.
        if (!encoded) { # Keep unexpected encoding failure visible to the host.
            return -1; # No fabricated response or hidden queue bypass is permitted.
        } # Finish response validation.
        output <-= response; # Block this Dis process until the receiver accepts its response.
        if (operation == 2) { # Shut down only after the final reply rendezvous completes.
            return 0; # Return ownership and teardown decisions to the embedding host.
        } # Finish acknowledged shutdown handling.
    } # Continue processing the next request.
} # Finish the teaching service.
