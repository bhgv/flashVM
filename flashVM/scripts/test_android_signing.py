#!/usr/bin/env python3
"""Check APK signing command contracts without an Android SDK or signing key."""

import importlib.util
from pathlib import Path
import secrets
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


SOURCE = (
    Path(__file__).resolve().parents[1] / "frontend/android/build_apk.py"
)  # Packager under test.
SPEC = importlib.util.spec_from_file_location(
    "flashvm_android_packager", SOURCE
)  # Isolated import.
PACKAGER = importlib.util.module_from_spec(SPEC)  # No packaging runs at import time.
SPEC.loader.exec_module(PACKAGER)


class SigningTests(unittest.TestCase):
    """Exercise required input, command privacy and existing-keystore reuse."""

    def test_reject_invalid_environment(self):
        """Reject absent, short and multiline values without echoing their contents."""
        for value in ("", "x", secrets.token_hex(8) + "\n", secrets.token_hex(8) + "\0"):
            with self.subTest(length=len(value)):
                with self.assertRaises(ValueError):
                    PACKAGER.signing_environment({"FVM_ANDROID_SIGNING_PASSWORD": value})

    def test_copy_environment(self):
        """Validation returns a separate environment and preserves the supplied value."""
        source = {"FVM_ANDROID_SIGNING_PASSWORD": secrets.token_hex(16)}
        result = PACKAGER.signing_environment(source)
        self.assertIsNot(result, source)
        self.assertEqual(result, source)

    def check_commands(self, existing):
        """Check one signing path; existing selects reuse. Returns recorded command lists."""
        secret = secrets.token_hex(16)
        environment = PACKAGER.signing_environment({"FVM_ANDROID_SIGNING_PASSWORD": secret})
        commands = []
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            if existing:
                (work / "test.keystore").write_bytes(b"fixture: never opened by a signing tool")
            args = SimpleNamespace(
                java_home=work / "jdk", build_tools=work / "sdk", output=work / "out.apk"
            )

            def record(arguments, child_environment):
                """Record borrowed SDK arguments and verify the child environment; return nothing."""
                self.assertEqual(child_environment["FVM_ANDROID_SIGNING_PASSWORD"], secret)
                command = list(map(str, arguments))
                self.assertTrue(all(secret not in argument for argument in command))
                commands.append(command)

            with patch.object(PACKAGER, "run", side_effect=record):
                PACKAGER.sign_apk(args, work / "aligned.apk", work, environment)
        return commands

    def test_new_identity(self):
        """New stores use keytool environment options, followed by apksigner environment options."""
        commands = self.check_commands(False)
        self.assertEqual(len(commands), 2)
        for flag in ("-storepass:env", "-keypass:env"):
            self.assertEqual(
                commands[0][commands[0].index(flag) + 1], "FVM_ANDROID_SIGNING_PASSWORD"
            )
        for flag in ("--ks-pass", "--key-pass"):
            self.assertEqual(
                commands[1][commands[1].index(flag) + 1], "env:FVM_ANDROID_SIGNING_PASSWORD"
            )

    def test_reuse_identity(self):
        """An existing store is signed without generating or replacing its key."""
        commands = self.check_commands(True)
        self.assertEqual(len(commands), 1)
        self.assertEqual(Path(commands[0][0]).name, "apksigner")


if __name__ == "__main__":
    unittest.main()
