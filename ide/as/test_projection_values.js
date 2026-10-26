/* Check the real optional AS client source; this is a client-contract oracle, not an SWF compiler. */
const fs = require("fs");
const vm = require("vm");


/* Execute the library with a capturing adapter and ensure helpers never inspect opaque receivers. */
function main() {
    const context = {};
    vm.createContext(context);
    vm.runInContext(fs.readFileSync(process.argv[2], "utf8"), context);
    const opaque = new Proxy({}, {get() {throw new Error("receiver was inspected");}});
    const calls = [];
    const done = function() {};
    const helpers = context.makeProjectionValues(function(operation, receiver, type, completion) {
        if (receiver !== opaque || completion !== done) {
            throw new Error("reference/completion changed");
        }
        calls.push([operation, type]);
        return "ticket";
    });
    const types = {tobyte: "byte", toint: "int", tobig: "big", toreal: "real", tofloat: "real",
                   tostring: "string", tobool: "bool", tochar: "char", totable: "table"};
    for (const name of Object.keys(types)) {
        if (helpers[name](opaque, done) !== "ticket") {
            throw new Error("adapter result was lost");
        }
        const call = calls[calls.length - 1];
        if (call[0] !== (name === "totable" ? "snapshot" : "readAs") || call[1] !== types[name]) {
            throw new Error("wrong Limbo type mapping: " + name);
        }
    }
    if (helpers.tofloat !== helpers.toreal) {
        throw new Error("tofloat is not the real alias");
    }
    process.stdout.write("PASS AS conversion client: Limbo type names, opaque references, real/float alias\n");
}

main();
