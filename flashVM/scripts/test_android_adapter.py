#!/usr/bin/env python3
"""Host regression checks for failure propagation and argument safety in the ADB adapter.

These use a fake transport and never count as Android runtime acceptance.
"""
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
import shlex
import subprocess
import tempfile
import threading
import unittest
from unittest.mock import patch

from android_test import run_test


class AdapterTests(unittest.TestCase):
    """Exercise device staging failures separately from target exit statuses."""

    def setUp(self):
        """Use an actual local target file without needing Android tools."""
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.binary = Path(self.temporary.name) / "target with spaces"
        self.binary.write_bytes(b"test fixture")

    def invoke(self, arguments=None, directory="/data/local/tmp/flashvm-test"):
        """Run one adapter operation against the current patched transport."""
        return run_test("adb", "explicit-device", directory, self.binary, arguments or [], 2)

    @patch("android_test.subprocess.run")
    def test_target_failure_and_quoted_arguments(self, run):
        """A nonzero remote result must survive argument quoting and staging."""
        run.side_effect = [subprocess.CompletedProcess([], 0)] * 3 + [
            subprocess.CompletedProcess([], 37),
            subprocess.CompletedProcess([], 0),
        ]
        arguments = ["space value", "a'b", "$(touch unwanted)", "; exit 0", ""]
        self.assertEqual(self.invoke(arguments), 37)
        calls = run.call_args_list
        self.assertEqual(len(calls), 5)
        for call in calls:
            self.assertEqual(call.args[0][:3], ["adb", "-s", "explicit-device"])
            self.assertEqual(call.kwargs["timeout"], 2)
        command = calls[-2].args[0]
        self.assertEqual(command[3:5], ["shell", "-T"])
        words = shlex.split(command[5])
        index = words.index("exec")
        remote = calls[2].args[0][-1]
        self.assertTrue(remote.startswith("/data/local/tmp/flashvm-test/" + self.binary.name + "-"))
        self.assertEqual(words[index + 1 :], [remote] + arguments)
        self.assertEqual(shlex.split(calls[-1].args[0][-1]), ["rm", "-f", remote])

    @patch("android_test.subprocess.run")
    def test_push_failure_stops_execution(self, run):
        """A staging failure must not execute an older device binary."""
        run.side_effect = [subprocess.CompletedProcess([], 0)] * 2 + [
            subprocess.CalledProcessError(1, "push"),
            subprocess.CompletedProcess([], 0),
        ]
        with self.assertRaises(subprocess.CalledProcessError):
            self.invoke()
        self.assertEqual(run.call_count, 4)
        self.assertEqual(
            shlex.split(run.call_args.args[0][-1]), ["rm", "-f", run.call_args_list[2].args[0][-1]]
        )

    @patch("android_test.subprocess.run")
    def test_repeated_target_uses_distinct_binaries_and_shared_cwd(self, run):
        """Two invocations share fixtures without pushing over an active binary."""
        run.return_value = subprocess.CompletedProcess([], 0)
        self.assertEqual(self.invoke(), 0)
        self.assertEqual(self.invoke(), 0)
        calls = run.call_args_list
        first, second = calls[2].args[0][-1], calls[7].args[0][-1]
        self.assertNotEqual(first, second)
        for index in (3, 8):
            words = shlex.split(calls[index].args[0][-1])
            self.assertEqual(words[words.index("cd") + 1], "/data/local/tmp/flashvm-test")

    @patch("android_test.subprocess.run")
    def test_parallel_target_staging_does_not_overwrite_active_binary(self, run):
        """Overlap remote executions and reject duplicate pushes like a real device."""
        barrier = threading.Barrier(2)
        lock = threading.Lock()
        staged = set()
        executed = set()

        def transport(command, **kwargs):
            if command[3] == "push":
                with lock:
                    self.assertNotIn(command[-1], staged)
                    staged.add(command[-1])
            elif command[3] == "shell":
                words = shlex.split(command[-1])
                if "exec" in words:
                    remote = words[words.index("exec") + 1]
                    with lock:
                        self.assertIn(remote, staged)
                        executed.add(remote)
                    barrier.wait(timeout=5)
                elif words[:2] == ["rm", "-f"]:
                    with lock:
                        staged.remove(words[-1])
            return subprocess.CompletedProcess(command, 0)

        run.side_effect = transport
        with ThreadPoolExecutor(max_workers=2) as pool:
            results = list(pool.map(lambda _: self.invoke(), range(2)))
        self.assertEqual(results, [0, 0])
        self.assertEqual(len(executed), 2)
        self.assertFalse(staged)

    @patch("android_test.subprocess.run")
    def test_cleanup_failure_cannot_pass_or_replace_target_failure(self, run):
        """Cleanup errors fail successes, while a target failure stays primary."""
        for result in (0, 37):
            run.side_effect = [subprocess.CompletedProcess([], 0)] * 3 + [
                subprocess.CompletedProcess([], result),
                subprocess.CompletedProcess([], 19),
            ]
            self.assertEqual(self.invoke(), result or 19)
            run.side_effect = [subprocess.CompletedProcess([], 0)] * 3 + [
                subprocess.CompletedProcess([], result),
                subprocess.TimeoutExpired("cleanup", 2),
            ]
            if result:
                self.assertEqual(self.invoke(), result)
            else:
                with self.assertRaises(subprocess.TimeoutExpired):
                    self.invoke()

    @patch("android_test.subprocess.run")
    def test_execution_timeout_cleans_its_binary_and_preserves_error(self, run):
        """A dead transport during cleanup must not hide the execution timeout."""
        error = subprocess.TimeoutExpired("execution", 2)
        run.side_effect = [subprocess.CompletedProcess([], 0)] * 3 + [
            error,
            subprocess.TimeoutExpired("cleanup", 2),
        ]
        with self.assertRaises(subprocess.TimeoutExpired) as caught:
            self.invoke()
        self.assertIs(caught.exception, error)
        self.assertEqual(
            shlex.split(run.call_args.args[0][-1]), ["rm", "-f", run.call_args_list[2].args[0][-1]]
        )

    @patch("android_test.subprocess.run")
    def test_device_timeout(self, run):
        """An unresponsive device must fail instead of reporting a skipped pass."""
        run.side_effect = subprocess.TimeoutExpired("adb", 2)
        with self.assertRaises(subprocess.TimeoutExpired):
            self.invoke()
        self.assertEqual(run.call_count, 1)

    @patch("android_test.subprocess.run")
    def test_unsafe_directory_is_rejected(self, run):
        """Do not stage into parent paths or shared device directories."""
        for directory in [
            "/data/local/tmp",
            "/data/local/tmp/flashvm-x/..",
            "/data/local/tmp/flashvm-",
            "/data/local/tmp/flashvm-x/child",
        ]:
            with self.assertRaises(ValueError):
                self.invoke(directory=directory)
        run.assert_not_called()

    @patch("android_test.subprocess.run")
    def test_missing_target_is_rejected(self, run):
        """A nonexistent build output must fail before touching the device."""
        self.binary.unlink()
        with self.assertRaises(FileNotFoundError):
            self.invoke()
        run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
