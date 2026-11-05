"""Independent bounded scalar call roles inferred only from decoded wire sections."""
from peer_call_binding import bindings, code_schema, type_catalog
from peer_call_schema import Reader, metadata

KINDS = {1: (4, 'Int32'), 2: (8, 'Big'), 3: (8, 'Real')}  # Distinct scalar kinds, not native WORD widths.


class Refusal(ValueError):
    """A violated rule in this bounded checkpoint, not a universal wire-invalid claim."""

    def __init__(self, stage, reason):
        """Store owned diagnostic strings; no publication or execution side effects."""
        super().__init__(reason)
        self.stage = stage


class Unsupported(Refusal):
    """A legal/unknown facility outside the checker, never automatic acceptance."""


def require(valid, stage, reason):
    """Raise a bounded refusal for false valid; return without effects otherwise."""
    if not valid:
        raise Refusal(stage, reason)


def catalog(payload):
    """Derive function intervals, signatures and frame roles from wire exports.

    Caller has checked structural bindings. No sidecar locals/budgets or private
    accept labels are consumed. Return owned decoded catalog; never execute code.
    """
    info = metadata(payload[1])
    if not info['profile']:
        raise Unsupported('module-role', 'library has no executable entry in this checkpoint')
    if 8 in payload:
        raise Unsupported('control', 'CONTROL semantics require a separate pass')
    if info['data_bytes'] or 4 in payload:
        raise Unsupported('module-data', 'ModuleData64/DATA initialization/layout requires a separate pass')
    if 5 in payload and bindings(payload[5], False):
        raise Unsupported('imports', 'cross-module resolution is outside this local-call checkpoint')
    source = Reader(payload[1])
    source.take(12)
    entry, frame = source.word(), source.word()
    source.finish()
    types = type_catalog(payload[3])
    functions = {}
    exports = bindings(payload[6], True) if 6 in payload else []
    for item in exports:
        pc = item['entry']
        signature = item['signature']
        if len(signature[1]) > 256:
            raise Unsupported('resource-limit', 'offline signature parameter ceiling')
        if signature[0] == 4 or 4 in signature[1]:
            raise Unsupported('signature-kind', 'Bool has no independent producer proof in this scalar subset')
        if pc in functions:
            require((functions[pc]['frame'], functions[pc]['signature']) == (item['frame'], signature),
                    'function-role', 'conflicting aliases at one function entry')
        else:
            functions[pc] = {'entry': pc, 'frame': item['frame'], 'signature': signature}
    require(entry in functions, 'module-entry', 'module entry is not an exported function start')
    require(functions[entry]['frame'] == frame, 'module-entry', 'module/export entry frame disagreement')
    require(functions[entry]['signature'] == (0, ()), 'module-entry', 'entry must have void/no-argument signature')
    code = code_schema(payload[2])
    starts = sorted(functions)
    if not starts or starts[0] != 0:
        raise Unsupported('function-ownership', 'unowned code prefix is outside the checkpoint')
    for index, pc in enumerate(starts):
        function = functions[pc]
        function['end'] = starts[index+1] if index+1 < len(starts) else len(code)
        function['bytes'] = types[function['frame']]['bytes']
        function['root'] = pc == entry
        function['outgoing'] = any(row['opcode'] == 4 for row in code[pc:function['end']])
        if any(types[function['frame']]['map']):
            raise Unsupported('frame-map', 'managed frame payload requires root/collector semantics')
        require(not function['outgoing'] or function['bytes'] >= 72,
                'frame-role', 'outgoing result tail overlaps Frame64 header')
        function['tail'] = function['bytes']-8 if function['outgoing'] else None
    return {'entry': entry, 'functions': functions, 'code': code, 'types': types,
            'unanalysed_type_ids': sorted(set(range(len(types)))-{f['frame'] for f in functions.values()})}


class Slots:
    """Per-function inferred fixed scalar spans, independent from initialization."""

    def __init__(self, size, tail=None):
        """Borrow scalar frame extent/tail offset; own an initially empty kind map."""
        self.size = size
        self.tail = tail
        self.kinds = {}

    def bind(self, offset, kind):
        """Constrain one exact scalar slot, refusing headers, overlaps and kind changes.

        This infers a kind only; it NEVER initializes a slot. Result tail is a
        separate role handled by the flow pass. Return the validated offset.
        """
        width, _ = KINDS[kind]
        require(offset >= 64 and offset % width == 0 and offset+width <= self.size,
                'local-role', 'scalar span crosses frame header/extent or alignment')
        if self.tail is not None:
            require(offset+width <= self.tail, 'result-role', 'ordinary scalar overlaps outgoing result tail')
        for prior, prior_kind in self.kinds.items():
            prior_width = KINDS[prior_kind][0]
            if offset < prior+prior_width and prior < offset+width:
                require(offset == prior and kind == prior_kind, 'local-kind', 'overlapping or reinterpreted scalar kinds')
        if offset not in self.kinds and len(self.kinds) >= 1024:
            raise Unsupported('resource-limit', 'offline inferred-slot ceiling')
        self.kinds[offset] = kind
        return offset

    def seed_parameters(self, signature):
        """Infer canonical signature prefix from byte64; return initialized offsets.

        Alignment follows each scalar width. No external local declarations or
        caller argument establishment is assumed by this function itself.
        """
        offset = 64
        result = set()
        for kind in signature[1]:
            width = KINDS[kind][0]
            offset = (offset+width-1)//width*width
            result.add(self.bind(offset, kind))
            offset += width
        return result
