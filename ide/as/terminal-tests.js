// Copyright (c) 2026 bhgv
// EN: Host algorithm acceptance of the actual AS bridge and original IDE codec; no movie execution claim.
// RU: Host algorithm acceptance AS bridge и original codec; не выполнение SWF movie.
const fs = require("fs");
const vm = require("vm");
const path = require("path");
const scope = {};
vm.createContext(scope);
vm.runInContext(fs.readFileSync(process.argv[2], "utf8"), scope);
vm.runInContext(fs.readFileSync(path.join(__dirname, "terminal-bridge.as"), "utf8"), scope);
vm.runInContext(fs.readFileSync(path.join(__dirname, "presentation.as"), "utf8"), scope);


// EN/RU: Fail explicitly on a violated contract. / Явный failure при нарушении contract.
function check(value, message)
{
    if (!value)
    {
        throw new Error(message);
    }
}
const sent = []; // Copied UI frames accepted by the mock host dispatcher.
const incoming = []; // One UI-only direction, independent of non-UI traffic.
let full = true; // Deterministic bounded queue pressure.
const terminal = {
    bidir: true,


    // EN/RU: Copy on acceptance, refuse without effects on pressure. / Copy при acceptance, отказ без effects.
    trySendFrame(frame)
    {
        if (full)
        {
            return false;
        }
        sent.push(frame);
        return true;
    },


    // EN/RU: Deliver one routed frame, never a batch. / Выдать один frame, не batch.
    tryReceiveFrame()
    {
        return incoming.shift();
    }
};
check(scope.makeIdeTerminalBridge({bidir: false}) === undefined, "ordinary terminal admitted");
check(scope.makeIdeTerminalBridge({bidir: true}) === undefined, "missing dispatcher admitted");
const codec = scope.makeIdeTerminalBridge(terminal);
check(codec !== undefined, "bidir bridge refused");
const request = {kind: 1, id: 17, code: 2, flags: 0, first: -1, second: 2, text: "A\u0000Я😀"};
check(codec.trySend(request) === false && sent.length === 0, "pressure lost acceptance semantics");
full = false;
check(codec.trySend(request) === true && sent.length === 1, "UI request not accepted exactly once");
incoming.push(sent[0]);
incoming.push(sent[0]);
const first = codec.tryReceive();
check(first.id === 17 && first.text === request.text && incoming.length === 1, "first UI frame changed or batched");
check(codec.tryReceive().first === -1 && incoming.length === 0, "second UI frame changed");
check(codec.tryReceive() === undefined, "empty input fabricated a frame");
check(scope.makeIdeTerminalPresentation(terminal, {}) !== undefined, "existing presentation not attached");
check(scope.makeIdeTerminalPresentation({bidir: false}, {}) === undefined, "ordinary presentation terminal admitted");
console.log("PASS existing IDE codec on explicit bidir dispatcher: pressure, one-frame delivery, Unicode/NUL");
