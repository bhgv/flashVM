// Load the generated arithmetic_as adapter before evaluating this implementation.
// This is a scalar bridge example; actual native Dis registration is still host work.


// Create callbacks for the exports described by arithmetic.m.
// Parameters: none. Returns the generated external-module dispatch capability.
// The AS heap owns handlers and closures; no raw Dis object/frame is exposed.
function makeArithmeticASModule() // Construct one independent module capability.
{ // Begin its private AS-owned state.
    var handlers = new Object(); // Map generated declaration IDs to implementation functions.


    // Resolve a declared export name instead of duplicating a numeric slot by hand.
    // Parameters: name is the .m export name; implementation is its AS callback.
    // Returns no value; throws if the generated interface does not contain that name.
    function bindExport(name, implementation) // Bind only this example's Arithmetic module.
    { // Begin the descriptor lookup.
        for (var i = 0; i < arithmetic_as_contract.exports.length; i++) // Visit generated exports in order.
        { // Inspect one descriptor without mutating the contract.
            var descriptor = arithmetic_as_contract.exports[i]; // Borrow the AS descriptor object.
            if (descriptor.module == "Arithmetic" && descriptor.name == name) // Require both identities.
            { // Publish the resolved callable only after finding its exact declaration.
                handlers["d" + descriptor.declaration] = implementation; // Match the adapter's lookup key.
                return; // Stop after binding the one declared entry.
            } // Continue searching when this descriptor is unrelated.
        } // No descriptor matched; do not invent a declaration ID.
        throw "export absent from arithmetic.m"; // Make an incorrect header/implementation pair visible.
    } // End bindExport; handlers retains the registered closure.


    // Implement add using the Limbo i32 value range and wraparound rule.
    // Parameters: args contains two marshaled numbers; request is borrowed metadata,
    // unused here. Returns the signed i32 sum; throws on invalid example arguments.
    bindExport("add", function(args, request) // Supply the function without calling it during registration.
    { // Validate again so standalone misuse of the example fails visibly.
        if (args.length != 2 || typeof args[0] != "number" || typeof args[1] != "number" || // Require two numbers.
            args[0] % 1 != 0 || args[1] % 1 != 0 || // Reject fractional inputs before bitwise conversion.
            args[0] < -2147483648 || args[0] > 2147483647 || // First argument must fit signed i32.
            args[1] < -2147483648 || args[1] > 2147483647) // Apply the same bound to the second argument.
        { // Reject arguments instead of silently coercing them.
            throw "expected two i32 values"; // The generated dispatcher converts this into an error reply.
        } // All admitted inputs now match this scalar implementation.
        return (args[0] + args[1]) | 0; // AS Number addition is wrapped explicitly to Limbo's signed 32 bits.
    }); // Store the callback under the descriptor's generated ID.


    // Implement greet without sharing string storage between engines.
    // Parameters: args contains one string; request is borrowed and unused metadata.
    // Returns a new AS string; the checked bridge must marshal the result value.
    bindExport("greet", function(args, request) // Register the second declared entry.
    { // Begin the string callback.
        if (args.length != 1 || typeof args[0] != "string") // Admit exactly the string case taught here.
        { // Report misuse before concatenating a value of another kind.
            throw "expected one string value"; // Do not pretend arbitrary composite marshaling exists.
        } // The argument remains owned by the AS heap while this callback executes.
        return "Hello, " + args[0]; // Return a managed AS string, never a native heap pointer.
    }); // Finish registration of greet.

    return arithmetic_as_module(handlers); // Wrap these functions with the generated request/reply checks.
} // The returned capability retains the handler table for its own lifetime.
