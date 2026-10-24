/* Optional host validation of the AS2 codec algorithm; not proof of FlashVM script execution. */
const fs = require("fs");
const vm = require("vm");
const path = require("path");
const scope = {};
vm.createContext(scope);
vm.runInContext(fs.readFileSync(path.join(__dirname, "../examples/ui-channel/bridge.as"), "utf8"), scope);
const codec = scope.makeUiBridge({}, {});
const message = {kind: 1, id: 17, code: 2, flags: 0, first: -2147483648,
                 second: 2147483647, text: "A\u0000Я😀"};
const encoded = codec.encode(message);
const header = "fvd1:00000001000000110000000200000000800000007fffffff";
if (encoded !== header + message.text)
{
    throw new Error("AS2 codec header differs from the C wire contract");
}
const decoded = codec.decode(encoded);
if (!decoded || decoded.first !== message.first || decoded.second !== message.second ||
    decoded.text !== message.text || decoded.id !== 17)
{
    throw new Error("AS2 codec lost signed bounds or Unicode/NUL payload");
}
for (const invalid of ["", encoded.slice(0, 52), encoded.slice(0, 6) + "g" + encoded.slice(7)])
{
    if (codec.decode(invalid) !== undefined)
    {
        throw new Error("AS2 decoder admitted malformed framing");
    }
}
for (const change of [{kind: "1"}, {id: 0}, {first: 2147483648}, {code: -1},
                      {second: NaN}, {text: "\ud800"}, {text: "😀".repeat(257)}])
{
    if (codec.encode(Object.assign({}, message, change)) !== undefined)
    {
        throw new Error("AS2 encoder admitted invalid fields or excess UTF-8 bytes");
    }
}
if (codec.encode(Object.assign({}, message, {text: "😀".repeat(256)})) === undefined)
{
    throw new Error("AS2 encoder rejected exactly 1024 UTF-8 bytes");
}
console.log("PASS AS2 wire algorithm: header, int32, Unicode/NUL, malformed input and byte quotas");
