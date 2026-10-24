// Load the generated arithmetic_as module adapter before this implementation.
// The .m header owns names/types; no raw Dis frame or heap reference appears here.
function makeArithmeticASModule()
{
    var handlers = new Object();

    // Bind by names from the generated descriptor, not a hand-maintained numeric slot.
    function bindExport(name, implementation)
    {
        for (var i = 0; i < arithmetic_as_contract.exports.length; i++) {
            var descriptor = arithmetic_as_contract.exports[i];
            if (descriptor.module == "Arithmetic" && descriptor.name == name) {
                handlers["d" + descriptor.declaration] = implementation;
                return;
            }
        }
        throw "export absent from arithmetic.m";
    }

    // The native bridge must already validate the resolved .m signature and values.
    // These explicit checks make the scalar example fail visibly if misused alone.
    bindExport("add", function(args, request) {
        if (args.length != 2 || typeof args[0] != "number" || typeof args[1] != "number" ||
            args[0] % 1 != 0 || args[1] % 1 != 0 ||
            args[0] < -2147483648 || args[0] > 2147483647 ||
            args[1] < -2147483648 || args[1] > 2147483647) {
            throw "expected two i32 values";
        }
        // Limbo int wraps at 32 bits. AS Number addition alone would not preserve it.
        return (args[0] + args[1]) | 0;
    });

    // Returned strings remain AS-owned until the bridge marshals the reply value.
    bindExport("greet", function(args, request) {
        if (args.length != 1 || typeof args[0] != "string") {
            throw "expected one string value";
        }
        return "Hello, " + args[0];
    });

    // Register this returned dispatch capability as an external Arithmetic endpoint.
    // Actual native Dis module registration is a separate host-bridge operation.
    return arithmetic_as_module(handlers);
}
