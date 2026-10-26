/* Execute the actual AS1/2-compatible decoder; this oracle is not a claim of AS source-to-SWF lowering. */
const fs = require("fs");
const vm = require("vm");
const path = require("path");
const environment = {};
vm.runInNewContext(fs.readFileSync(path.join(__dirname, "diagnostics.as"), "utf8"), environment);
const frame = fs.readFileSync(process.argv[2], "ascii");
const decoded = environment.decodeBrokerDiagnostic(frame);
if (!decoded || decoded.kind !== 1 || decoded.requestId !== "ffffffffffffffff"
    || decoded.origin.path !== process.argv[3] || decoded.peer !== "tcp://127.0.0.1:9001") {
    throw new Error("AS diagnostic lost exact ID/Unicode/source/endpoint");
}
for (let size = 0; size < frame.length; size++) {
    if (environment.decodeBrokerDiagnostic(frame.slice(0, size)) !== undefined) {
        throw new Error("AS accepted truncated diagnostic at " + size);
    }
}
if (environment.decodeBrokerDiagnostic(frame + "0") !== undefined) {
    throw new Error("AS accepted trailing diagnostic bytes");
}
let received = 0;
const client = environment.makeBrokerDiagnostics({tryReceive: function () {return received++ ? undefined : frame;}});
if (!client.tryReceive() || client.tryReceive() !== undefined) {
    throw new Error("AS channel helper changed empty/nonblocking semantics");
}
process.stdout.write(JSON.stringify(decoded));
