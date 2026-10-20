#!/usr/bin/env python3
"""Deterministic standalone SWF8 fixture writer; not a language compiler."""
from pathlib import Path  # Resolve output beside this tutorial, independently of cwd.
import struct  # Encode wire integers explicitly little endian, without C struct padding.


def tag(code, payload=b''):  # Serialize one SWF tag, selecting its short or long length form.
    """Return a complete bounded tag record for this small authored movie."""
    if len(payload) < 63:  # The six low header bits can hold short payload lengths.
        return struct.pack('<H', code << 6 | len(payload)) + payload  # Inline short length.
    return struct.pack('<HI', code << 6 | 63, len(payload)) + payload  # Explicit long length.


def push(value):  # Encode only the string/int types required by this tutorial.
    """Return one complete AVM1 ActionPush record."""
    if isinstance(value, str):  # Script names and method names are ASCII here.
        payload = b'\0' + value.encode('ascii') + b'\0'  # String type plus NUL terminator.
    else:  # Remaining values are explicit signed 32-bit integer operands.
        payload = b'\x07' + struct.pack('<i', value)  # Integer Push type and its value.
    return b'\x96' + struct.pack('<H', len(payload)) + payload  # Action code and payload length.


def variable(name):  # Read an AVM1 variable by its explicit script name.
    """Push the named value through ActionGetVariable."""
    return push(name) + b'\x1c'  # Name then GetVariable, leaving one result.


def call(method, arguments):  # Call a Mouse namespace method with ordered arguments.
    """Return a stack-balanced Mouse method call except for its returned value."""
    operands = b''.join(reversed(arguments))  # AVM1 consumes arguments from the stack top.
    return operands + push(len(arguments)) + variable('Mouse') + push(method) + b'\x52'  # CallMethod.


def define(name, parameters, body):  # Give each callback an explicit named parameter list.
    """Serialize DefineFunction and preserve its exact embedded-code length."""
    header = name.encode('ascii') + b'\0' + struct.pack('<H', len(parameters))  # Name/count.
    header += b''.join(p.encode('ascii') + b'\0' for p in parameters)  # Parameter names.
    payload = header + struct.pack('<H', len(body)) + body  # Complete function payload.
    return b'\x9b' + struct.pack('<H', len(payload)) + payload  # DefineFunction action.


def movie():  # Assemble the same behavior explained by the companion ActionScript.
    """Build one stopped SWF8 frame whose listeners remain live for host input."""
    actions = push('observer') + push(0) + b'\x43\x1d'  # Create and store one listener object.
    callbacks = [('wheel', ['delta'], variable('delta') + b'\x26\x00', 'onMouseWheel'),  # Trace lines.
                 ('down', [], call('hide', []) + b'\x17\x00', 'onMouseDown'),  # Hide on press.
                 ('up', [], call('show', []) + b'\x17\x00', 'onMouseUp')]  # Show on release.
    for name, parameters, body, event in callbacks:  # Define each function before installing it.
        actions += define(name, parameters, body)  # Register a named AVM1 function value.
        actions += variable('observer') + push(event) + variable(name) + b'\x4f'  # SetMember.
    actions += call('addListener', [variable('observer')]) + b'\x17'  # Register once and discard bool.
    actions += b'\x07\x00'  # Stop timeline, then ActionEnd; the host still services input.
    bits = '10000' + ''.join(format(v & 65535, '016b') for v in (0, 6400, 0, 4000))  # RECT twips.
    bits += '0' * (-len(bits) % 8)  # Pad the bit-packed rectangle to an octet boundary.
    rect = bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))  # Big-endian bit groups.
    body = rect + struct.pack('<HH', 24 * 256, 1)  # Authored 24 fps, one 320×200 frame.
    body += tag(9, b'\x30\x50\x70') + tag(12, actions) + tag(1) + tag(0)  # Background/actions/end.
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body  # Uncompressed SWF8 file length.


if __name__ == '__main__':  # Importing for tests has no output/file-writing side effects.
    Path(__file__).with_name('wheel-cursor.swf').write_bytes(movie())  # Publish the deterministic asset.
