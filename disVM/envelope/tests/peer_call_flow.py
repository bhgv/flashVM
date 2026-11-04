"""Independent straight-line scalar FRAME/CALL/result/LEA flow; never executes."""
from peer_call_roles import Slots, KINDS, Refusal, Unsupported, require

MOVES = {45: 1, 118: 2, 46: 3}  # Existing MOVW/MOVL/MOVF wire IDs and scalar kinds.
ARITHMETIC = {58, 61, 64, 67}  # Existing Int32 ADD/SUB/MUL/DIV; no value evaluation.


def check_function(function, world):
    """Infer scalar kinds/initialization and verify local calls for one interval.

    Supports straight-line root-to-leaf composition only. Return owned facts;
    no runtime value, root, pointer, machine code or execution admission is made.
    """
    slots = Slots(function['bytes'], function['tail'])
    initialized = slots.seed_parameters(function['signature'])
    prepared = None
    result_kind = None
    returned = False
    calls = []

    def read(offset, kind):
        """Require a typed initialized local/result-tail without changing state."""
        if offset == function['tail']:
            require(result_kind == kind, 'initialization', 'outgoing result is absent or has a different kind')
        else:
            slots.bind(offset, kind)
            require(offset in initialized, 'initialization', 'scalar read before definite initialization')

    def source(row, kind):
        """Validate immediate or direct FP source; unknown addressing stays unsupported."""
        mode = (row['mode'] >> 3) & 7
        if mode == 2:
            if kind == 3:
                raise Unsupported('real-immediate', 'raw REAL immediate representation is not inferred')
        elif mode == 1:
            read(row['source'], kind)
        else:
            raise Unsupported('scalar-source', 'source addressing is outside direct scalar checkpoint')

    def write(row, kind):
        """Publish an abstract typed local, prepared argument or callee return value."""
        nonlocal returned
        mode = row['mode'] & 7
        if mode == 1:
            initialized.add(slots.bind(row['destination'], kind))
        elif mode == 5:
            if row['destination'] == 40:
                require(prepared is not None, 'prepared-frame', 'argument store without a live prepared frame')
                offset = prepared['slots'].bind(row['secondary'], kind)
                prepared['initialized'].add(offset)
            elif row['destination'] == 32 and row['secondary'] == 0:
                require(not function['root'] and function['signature'][0] == kind,
                        'result-kind', 'return store width/kind disagrees with signature')
                returned = True
            else:
                raise Refusal('result-role', 'indirect scalar destination is not an argument or return slot')
        else:
            raise Unsupported('scalar-destination', 'destination addressing is outside checkpoint')

    for pc in range(function['entry'], function['end']):
        row = world['code'][pc]
        opcode = row['opcode']
        try:
            if row['mode'] & 192:
                raise Unsupported('middle-operand', 'explicit middle operand requires a separate transfer rule')
            if opcode in MOVES:
                kind = MOVES[opcode]
                source(row, kind)
                write(row, kind)
            elif opcode in ARITHMETIC:
                source(row, 1)
                require((row['mode'] & 7) == 1, 'arithmetic-role', 'Int32 RMW requires a direct local destination')
                read(row['destination'], 1)
                initialized.add(slots.bind(row['destination'], 1))
            elif opcode == 5:
                if row['mode'] != 17 or row.get('destination') != 40:
                    raise Unsupported('frame-role', 'only immediate FRAME into header temporary40 is supported')
                require(function['root'], 'call-role', 'nested helper calls are outside the root-to-leaf contract')
                require(prepared is None, 'prepared-frame', 'new FRAME abandons an unconsumed preparation')
                type_id = row['source']
                prepared = {'type': type_id, 'slots': Slots(world['types'][type_id]['bytes']),
                            'initialized': set(), 'result_address': False}
                result_kind = None
            elif opcode == 39:
                require(row['mode'] == 13 and row.get('destination') == 40 and row.get('secondary') == 32,
                        'lea-role', 'checked result LEA must target prepared-frame byte32')
                require(prepared is not None, 'lea-role', 'result LEA without prepared frame')
                require(row['source'] == function['tail'], 'lea-role', 'result LEA does not address exact outgoing tail')
                prepared['result_address'] = True
            elif opcode == 4:
                require(row['mode'] == 10 and row.get('source') == 40, 'call-role', 'CALL must consume prepared temporary40')
                require(prepared is not None, 'call-role', 'CALL without a prepared frame')
                target = row['destination']
                require(target in world['functions'], 'function-ownership', 'CALL target is not a function entry')
                callee = world['functions'][target]
                require(not callee['root'] and function['root'], 'call-role', 'only root-to-leaf calls supported')
                require(prepared['type'] == callee['frame'], 'frame-signature', 'prepared/callee frame disagreement')
                expected = Slots(callee['bytes'])
                arguments = expected.seed_parameters(callee['signature'])
                require(set(prepared['slots'].kinds) <= arguments,
                        'argument-role', 'prepared scalar write is outside canonical parameter prefix')
                for offset in arguments:
                    require(prepared['slots'].kinds.get(offset) == expected.kinds[offset] and
                            offset in prepared['initialized'], 'argument-kind/init', 'missing or mismatched argument')
                require(not callee['signature'][0] or prepared['result_address'],
                        'result-address', 'nonvoid call has no checked result address')
                result_kind = callee['signature'][0] or None
                calls.append({'pc': pc, 'target': target, 'result_kind': result_kind})
                prepared = None
            elif opcode in (12, 15):
                require(pc == function['end']-1, 'function-ownership', 'terminal before interval end')
                require(opcode == (15 if function['root'] else 12), 'terminal-role', 'EXIT/RET disagrees with function role')
                require(prepared is None, 'prepared-frame', 'terminal abandons prepared frame')
                require(function['root'] or not function['signature'][0] or returned,
                        'return-initialization', 'nonvoid RET without typed result store')
            else:
                raise Unsupported('opcode/CFG', 'no semantic transfer for opcode '+str(opcode))
        except Refusal as failure:
            raise type(failure)(failure.stage, 'pc '+str(pc)+': '+str(failure)) from failure
    require(world['code'][function['end']-1]['opcode'] in (12, 15),
            'function-ownership', 'function falls through its owned interval')
    return {'entry': function['entry'], 'end': function['end'], 'frame': function['frame'],
            'role': 'module-entry' if function['root'] else 'helper',
            'signature': function['signature'], 'inferred_slots': sorted(slots.kinds.items()),
            'calls': calls, 'typed_return_store': returned, 'outgoing_tail': function['tail']}


def check_flow(world):
    """Check every function before reporting any call/result summary as passed.

    No manifest locals are consumed. Abstract call results stand only for normal
    successful return paths; division-zero/termination/lifetimes are not proved.
    """
    return [check_function(world['functions'][pc], world) for pc in sorted(world['functions'])]
