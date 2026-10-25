"""Execute generated common-AS source in Node to test transport contracts.

The Dis endpoint below is a process double. This is neither SWF execution nor
acceptance of a native Dis/SWF module loader or marshaler.
"""

from pathlib import Path
import subprocess
import sys
import tempfile


# Generate both directions from exactly one .m snapshot, then run the actual output.
def main():
    generator, node = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="limbo-as-bridge-") as directory:
        root = Path(directory)
        header = root / "native.m"
        header.write_text("Native:module{add:fn(a,b:int):int; echo:fn(s:string):string;};")
        source = []
        for direction, prefix in [("as-module", "server"), ("as-proxy", "client")]:
            output = root / (prefix + ".as")
            subprocess.run(
                [generator, "--format", direction, str(header), str(output), "--prefix", prefix],
                check=True,
            )
            source.append(output.read_text())
        harness = r"""
const assert = require('assert');
assert.strictEqual(server_contract.interface_id, client_contract.interface_id);
const add = server_contract.exports.filter(x => x.name === 'add')[0];
let calls = 0;
const handlers = {};
handlers['d' + add.declaration] = function(args) {
    // A real native bridge must validate the resolved .m types before delivery.
    assert.strictEqual(args.length, 2);
    assert.strictEqual(typeof args[0], 'number');
    assert.strictEqual(typeof args[1], 'number');
    calls++;
    return args[0] + args[1];
};
const external = server_module(handlers);
const request = {schema:1, engine:'as', interface_id:server_contract.interface_id,
                 id:7, declaration:add.declaration, module:'Native', name:'add', args:[3,4]};
let replies = [];
external.dispatch(request, r => replies.push(r));
assert.strictEqual(calls, 1);
assert.strictEqual(replies.length, 1);
assert.strictEqual(replies[0].engine, 'as');
assert.strictEqual(replies[0].value, 7);
for (const bad of [Object.assign({}, request, {interface_id:'stale'}),
                   Object.assign({}, request, {name:'other'}),
                   Object.assign({}, request, {args:null}),
                   Object.assign({}, request, {declaration:String(add.declaration)})]) {
    external.dispatch(bad, r => replies.push(r));
    assert.strictEqual(replies[replies.length - 1].status, 'error');
}
assert.strictEqual(calls, 1);
// Primitive guards reject before executing an external callback.
for (const args of [[], [1], [1,2,3], ['1',2], [1.5,2], [NaN,2], [Infinity,2], [2147483648,0]]) {
    external.dispatch(Object.assign({}, request, {args}), r => assert.strictEqual(r.status, 'error'));
}
assert.strictEqual(calls, 1);
handlers['d' + add.declaration] = () => 'wrong result';
external.dispatch(request, r => assert.strictEqual(r.error, 'result type mismatch'));
handlers['d' + add.declaration] = args => (args[0] + args[1]) | 0;
external.dispatch(Object.assign({}, request, {args:[2147483647,1]}), r => assert.strictEqual(r.value,-2147483648));
server_module(null).dispatch(request, r => {
    assert.strictEqual(r.status, 'error');
    assert.ok(r.error.indexOf('unimplemented') >= 0);
});
// AS -> Dis: queue requests in a fake checked endpoint, then deliver replies.
let pending = [];
const proxy = client_proxy((request, receive) => pending.push({request, receive}));
let completed = [];
const args = [5,6];
const first = proxy.call('Native', 'add', args, (value,error) => completed.push({value,error}));
args[0] = 99;
assert.strictEqual(first, 1);
assert.deepStrictEqual(pending[0].request.args, [5,6]);
const sent = pending[0].request;
assert.strictEqual(sent.engine, 'dis');
const response = {schema:1, engine:'dis', interface_id:client_contract.interface_id,
                  id:sent.id, declaration:sent.declaration, status:'ok', value:11, error:null};
pending[0].receive(Object.assign({}, response, {id:999}));
pending[0].receive(Object.assign({}, response, {engine:'as'}));
pending[0].receive(Object.assign({}, response, {interface_id:'old'}));
assert.strictEqual(completed.length, 0);
pending[0].receive(response);
pending[0].receive(response);
assert.deepStrictEqual(completed, [{value:11,error:null}]);
assert.strictEqual(proxy.callById(add.declaration, [1,2], (v,e) => completed.push({value:v,error:e})), 2);
const second = pending[1].request;
pending[1].receive(Object.assign({}, response, {id:second.id,status:'error',value:null,error:'typed call rejected'}));
assert.strictEqual(completed[1].error, 'typed call rejected');
assert.throws(() => proxy.call('Native', 'missing', [], () => {}));
assert.throws(() => proxy.callById(String(add.declaration), [], () => {}));
// Invalid caller arguments never reach transport or consume correlation IDs.
for (const args of [[1], ['1',2], [0.5,2], [1,Infinity]]) {
    assert.throws(() => proxy.callById(add.declaration, args, () => {}), /argument type/);
}
assert.strictEqual(pending.length, 2);
const third = proxy.callById(add.declaration, [1,2], (v,e) => completed.push({value:v,error:e}));
assert.strictEqual(third, 3);
pending[2].receive(Object.assign({}, response, {id:third,value:'bad'}));
pending[2].receive(Object.assign({}, response, {id:third,value:3}));
assert.deepStrictEqual(completed[2], {value:null,error:'result type mismatch'});
const failed = client_proxy(() => { throw 'transport failed'; });
let errors = [];
failed.call('Native','add',[1,2],(v,e) => errors.push(e));
assert.deepStrictEqual(errors, ['transport failed']);
console.log('PASS .m AS external implementation and Dis proxy transport contracts');
"""
        script = root / "check.js"
        script.write_text("\n".join(source) + "\n" + harness)
        subprocess.run([node, str(script)], check=True, timeout=20)


if __name__ == "__main__":
    main()
