#!/usr/bin/env python3
"""Host regression checks for failure propagation and argument safety in the ADB adapter.

These use a fake transport and never count as Android runtime acceptance.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from android_test import run_test


class AdapterTests(unittest.TestCase):
    """Exercise device staging failures separately from target exit statuses."""

    def setUp(self):
        """Use an actual local target file without needing Android tools."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.binary = Path(self.temporary.name) / 'target with spaces'
        self.binary.write_bytes(b'test fixture')

    def invoke(self, arguments=None, directory='/data/local/tmp/flashvm-test'):
        """Run one adapter operation against the current patched transport."""
        return run_test('adb', 'explicit-device', directory, self.binary, arguments or [], 2)

    @patch('android_test.subprocess.run')
    def test_target_failure_and_quoted_arguments(self, run):
        """A nonzero remote result must survive argument quoting and staging."""
        run.side_effect = [subprocess.CompletedProcess([], 0)] * 3 + [subprocess.CompletedProcess([], 37)]
        arguments = ['space value', "a'b", '$(touch unwanted)', '; exit 0', '']
        self.assertEqual(self.invoke(arguments), 37)
        calls = run.call_args_list
        self.assertEqual(len(calls), 4)
        for call in calls:
            self.assertEqual(call.args[0][:3], ['adb', '-s', 'explicit-device'])
            self.assertEqual(call.kwargs['timeout'], 2)
        command = calls[-1].args[0]
        self.assertEqual(command[3:5], ['shell', '-T'])
        words = shlex.split(command[5])
        index = words.index('exec')
        self.assertEqual(words[index + 1:], ['/data/local/tmp/flashvm-test/' + self.binary.name] + arguments)

    @patch('android_test.subprocess.run')
    def test_push_failure_stops_execution(self, run):
        """A staging failure must not execute an older device binary."""
        run.side_effect = [subprocess.CompletedProcess([], 0)] * 2 + [subprocess.CalledProcessError(1, 'push')]
        with self.assertRaises(subprocess.CalledProcessError):
            self.invoke()
        self.assertEqual(run.call_count, 3)

    @patch('android_test.subprocess.run')
    def test_device_timeout(self, run):
        """An unresponsive device must fail instead of reporting a skipped pass."""
        run.side_effect = subprocess.TimeoutExpired('adb', 2)
        with self.assertRaises(subprocess.TimeoutExpired):
            self.invoke()
        self.assertEqual(run.call_count, 1)

    @patch('android_test.subprocess.run')
    def test_unsafe_directory_is_rejected(self, run):
        """Do not stage into parent paths or shared device directories."""
        for directory in ['/data/local/tmp', '/data/local/tmp/flashvm-x/..',
                          '/data/local/tmp/flashvm-', '/data/local/tmp/flashvm-x/child']:
            with self.assertRaises(ValueError):
                self.invoke(directory=directory)
        run.assert_not_called()

    @patch('android_test.subprocess.run')
    def test_missing_target_is_rejected(self, run):
        """A nonexistent build output must fail before touching the device."""
        self.binary.unlink()
        with self.assertRaises(FileNotFoundError):
            self.invoke()
        run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
