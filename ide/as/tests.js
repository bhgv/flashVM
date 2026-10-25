/* Optional host algorithm checks; the product is AS, not this Node test harness. */
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const scope = {};
vm.createContext(scope);
vm.runInContext(fs.readFileSync(path.join(__dirname, "syntax-worker.as"), "utf8"), scope);
const worker = scope.makeIdeSyntaxWorker();
const job = {source: 'var имя = "😀"; /* comment */\nreturn 42;', language: "as", revision: 7,
             base: 20, ticket: 11, state: "normal", escaped: false, star: false};
const result = worker.scan(job);
if (!result.ok || result.revision !== 7 || result.state !== "normal" ||
    result.spans.filter(s => s.style === "keyword").length !== 2 ||
    !result.spans.some(s => s.style === "comment") ||
    !result.spans.some(s => s.style === "number"))
{
    throw new Error("AS syntax window missed a keyword, comment, literal or revision");
}
let previous = job.base;
for (const span of result.spans)
{
    if (span.first < previous || span.last <= span.first || span.last > job.base + result.length)
    {
        throw new Error("AS syntax output overlaps or escapes the source window");
    }
    const units = job.source.slice(span.unitFirst, span.unitLast);
    if (Array.from(units).length !== span.last - span.first)
    {
        throw new Error("AS colour spans disagree with Dis Unicode coordinates");
    }
    previous = span.last;
}
const left = worker.scan(Object.assign({}, job, {source: "/* split *", base: 0}));
const right = worker.scan(Object.assign({}, job, {source: "/ var z = 2;", base: left.length,
    state: left.state, escaped: left.escaped, star: left.star}));
if (!left.ok || !left.star || !right.ok || right.state !== "normal" ||
    !right.spans.some(s => s.style === "keyword"))
{
    throw new Error("AS scanner lost a block delimiter split across windows");
}
for (const change of [{source: "\ud800"}, {base: -1}, {revision: NaN},
                      {source: "x".repeat(16385)}, {state: "unknown"}])
{
    if (worker.scan(Object.assign({}, job, change)).ok)
    {
        throw new Error("AS scanner admitted an invalid or oversized job");
    }
}
const limbo = worker.scan(Object.assign({}, job, {language: "limbo", source: "spawn task(); # comment"}));
if (!limbo.ok || limbo.spans[0].style !== "keyword" ||
    limbo.spans[limbo.spans.length - 1].style !== "comment")
{
    throw new Error("Limbo lexical vocabulary/comment rule failed");
}
console.log("PASS AS syntax worker: revision, Unicode spans, bounded windows and continued comments");
const codecSource = process.argv[2];
if (!codecSource)
{
    throw new Error("Pass the explicit shared AS codec source path as the first argument");
}
vm.runInContext(fs.readFileSync(codecSource, "utf8"), scope);
vm.runInContext(fs.readFileSync(path.join(__dirname, "presentation.as"), "utf8"), scope);
const codec = scope.makeUiBridge({}, {});
const displayed = {documents: [], selections: [], syntax: [], completion: [], calltip: [], errors: []};
const presentation = scope.makeIdePresentation(codec, {
    document: (...args) => displayed.documents.push(args),
    selection: (...args) => displayed.selections.push(args),
    syntax: value => { displayed.syntax.push(value); return true; },
    completion: value => { displayed.completion.push(value); return true; },
    calltip: value => { displayed.calltip.push(value); return true; },
    fault: value => displayed.errors.push(value)
});
const hex = n => (n >>> 0).toString(16).padStart(8, "0");
const metadata = (offset, total) => [-1, -1, -1, -1, -1, offset, total].map(hex).join("");
const snapshot = (revision, offset, total, text) => ({kind: 7, id: 0, code: 0,
    flags: revision, first: 1, second: 1, text: metadata(offset, total) + text});
const doc = "var имя = \"😀\";";
const boundary = 4;
const firstPart = doc.slice(0, boundary);
const secondPart = doc.slice(boundary);
const total = Buffer.byteLength(doc, "utf8");
if (!presentation.accept(snapshot(1, 0, total, firstPart)) || displayed.documents.length !== 0 ||
    !presentation.accept(snapshot(1, Buffer.byteLength(firstPart), total, secondPart)) ||
    displayed.documents.length !== 1 || displayed.documents[0][0] !== doc)
{
    throw new Error("AS presentation exposed partial text or lost fragmented Unicode");
}
if (!presentation.accept({kind: 7, id: 0, code: 65536, flags: 1, first: 2, second: 2, text: ""}) ||
    displayed.selections.length !== 1)
{
    throw new Error("AS presentation did not apply the Dis-owned selection");
}
const coloured = worker.scan({source: doc, language: "as", revision: 1, base: 0,
    ticket: 10, state: "normal", escaped: false, star: false});
presentation.expectSyntax({revision: 1, base: 0, ticket: 10});
if (!presentation.applySyntax(coloured) ||
    presentation.applySyntax(Object.assign({}, coloured, {ticket: 9})))
{
    throw new Error("AS presentation accepted an old lexical job or rejected the current job");
}
presentation.expectCompletion({revision: 1, generation: 3, prefix: "pri", ticket: 11});
const candidates = {revision: 1, generation: 3, prefix: "pri", ticket: 11,
    items: [{name: "print", kind: 2, signature: "print(string)", documentation: "output"}]};
if (!presentation.applyCompletion(candidates) ||
    presentation.applyCompletion(Object.assign({}, candidates, {generation: 2})))
{
    throw new Error("AS completion presentation did not gate index generations");
}
if (!presentation.accept(snapshot(2, 0, total, doc)) || presentation.applySyntax(coloured) ||
    presentation.applyCompletion(candidates))
{
    throw new Error("AS presentation applied stale independent-worker output after a new revision");
}
console.log("PASS AS presentation: atomic snapshots, Dis selection, syntax tickets and completion generations");
for (const [source, expectedName, expectedArgument] of [
    ["Math.max(1, ", "Math.max", 1],
    ['MovieClip.hitTest(1, "a,b", ', "MovieClip.hitTest", 2],
    ["outer([1, 2], inner(3, 4), ", "outer", 2],
    ["Sys.open(\"/path\", ", "Sys.open", 1]
])
{
    const context = worker.context(Object.assign({}, job, {source}));
    if (!context.ok || !context.call || context.call.name !== expectedName ||
        context.call.argument !== expectedArgument)
    {
        throw new Error("AS assistance context lost nested/literal argument boundaries: " + source);
    }
}
const member = worker.context(Object.assign({}, job, {source: "Math.ma"}));
const emptyMember = worker.context(Object.assign({}, job, {source: "MovieClip."}));
if (!member.ok || member.prefix.qualified !== "Math.ma" || member.prefix.text !== "ma" ||
    !emptyMember.ok || emptyMember.prefix.qualified !== "MovieClip." || emptyMember.prefix.text !== "")
{
    throw new Error("AS assistance did not expose qualified/empty member prefixes");
}
console.log("PASS AS/Limbo lexical assistance: members, nested calls and active parameter index");

const limboCall = worker.context(Object.assign({}, job, {language: "limbo", source: 'Sys->open("/path", '}));
if (!limboCall.ok || limboCall.call.name !== "Sys.open" || limboCall.call.argument !== 1)
{
    throw new Error("Limbo module arrow was not resolved to the canonical catalog name");
}
presentation.expectCalltip({revision: 2, generation: 3, name: "Math.max", argument: 1, ticket: 12});
const calltip = {revision: 2, generation: 3, name: "Math.max", argument: 1, ticket: 12,
    tips: [{name: "Math.max", signature: "Math.max(...values:Number):Number", active: 0,
        parameter: {name: "...values", type_name: "Number", optional: 1, summary: "Numeric arguments."}}]};
if (!presentation.applyCalltip(calltip) ||
    presentation.applyCalltip(Object.assign({}, calltip, {argument: 0})))
{
    throw new Error("AS parameter help failed the exact active-call context gate");
}
console.log("PASS AS/Limbo call tips: module access and exact active argument context");
