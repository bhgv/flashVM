// Load the generated arithmetic_dis proxy before evaluating this caller example.
// sendToDis must be a checked native channel/transport capability, not a UI shortcut.


// Request Arithmetic.add through the descriptor generated from arithmetic.m.
// Parameters: sendToDis accepts a request value and a reply callback.
// Returns a correlation ID local to this proxy; completion arrives asynchronously.
function useArithmeticDis(sendToDis) // The host explicitly supplies the transport capability.
{ // Begin one independently owned proxy instance.
    var arithmetic = arithmetic_dis_proxy(sendToDis); // Bind .m identities and validate transport kind.


    // Handle completion without blocking the AS event/render owner.
    // Parameters: value is the marshaled result; error is null on success.
    // Returns nothing; trace reports the result or the actual transport/module error.
    var callId = arithmetic.call("Arithmetic", "add", [6, 7], function(value, error) // Request the declared scalar entry.
    { // Begin the reply callback retained by the pending transport operation.
        if (error != null) // An error reply must not be interpreted as a numeric result.
        { // Report the failure while the copied error value remains valid.
            trace("Arithmetic.add failed: " + error); // Send the diagnostic to the host trace callback.
            return; // Finish this failed completion without printing a result.
        } // Successful completion has a validated result value instead.
        trace(value); // Expected value is 13 when the supplied Dis implementation performs addition.
    }); // Queue/send through the explicit transport and obtain this call's ID.

    return callId; // The host retains pending-call roots until completion, error or explicit cancellation.
} // The ID is not a global address and is meaningful only for this proxy instance.
