"""Deterministic process doubles: orchestration evidence, not language acceptance."""
import importlib.util
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("ide_build", ROOT / "build.py")
driver = importlib.util.module_from_spec(spec)
spec.loader.exec_module(driver)


class BuildCase(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix="ide-test-")
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        self.source = self.root / "program.as"
        self.source.write_text("trace(42);\n", encoding="utf-8")
        self.output = self.root / "movie.swf"
        self.output.write_bytes(b"previous artifact")
        self.configure()

    def configure(self, **config):
        self.tools = []
        for role in ("source", "ir", "link", "run"):
            path = self.root / (role + ".py")
            code = "#!" + sys.executable + "\n" + "CONFIG = " + repr(config) + "\nROLE = " + repr(role) + "\n" + r"""
import os, struct, sys, time
from pathlib import Path
stage = ROLE
if ROLE == 'ir':
    stage = {'check': 'verify_optimised' if Path(sys.argv[2]).name == 'optimised.fir' else 'verify', 'opt':'optimise', 'object':'object'}[sys.argv[1]]
if CONFIG.get('log') == stage:
    print('X' * 70000)
if CONFIG.get('sleep') == stage:
    time.sleep(5)
if CONFIG.get('fail') == stage:
    print(CONFIG.get('error', 'deliberate failure'), file=sys.stderr)
    sys.exit(3)
if ROLE == 'run':
    print(CONFIG.get('trace', '42'))
    sys.exit(0)
if ROLE == 'ir' and sys.argv[1] == 'check':
    sys.exit(0)
path = Path(sys.argv[4] if ROLE == 'source' else sys.argv[2] if ROLE == 'link' else sys.argv[3])
if CONFIG.get('missing') == stage:
    sys.exit(0)
body = b'FIR 1.0\n'
if stage == 'object': body = b'SWO1payload'
if stage == 'link': body = b'FWS\x08' + struct.pack('<I', 16) + b'\0' * 8
if CONFIG.get('malformed') == stage: body = b'bad output'
if CONFIG.get('bad_length') and stage == 'link': body = b'FWS\x08' + struct.pack('<I', 99) + b'\0' * 8
if CONFIG.get('empty') == stage: body = b''
path.write_bytes(body)
"""
            path.write_text(code, encoding="utf-8")
            path.chmod(0o755)
            self.tools.append(path)

    def build(self, **kwargs):
        args = dict(source=self.source, output=self.output, language="as2", compiler=self.tools[0], ir=self.tools[1], linker=self.tools[2])
        args.update(kwargs)
        return driver.build(**args)

    def rejected(self, result, stage):
        self.assertFalse(result['ok'], result)
        self.assertFalse(result['published'])
        self.assertEqual(result['diagnostics'][0]['stage'], stage)
        self.assertEqual(self.output.read_bytes(), b'previous artifact')
        self.assertFalse(list(self.root.glob('.ide-build-*')))
