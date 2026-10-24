from support import BuildCase


# Group related build scenarios with independent temporary ownership.
class TestCli(BuildCase):
    # Verify cli json and exit status without publishing an invalid build.
    def test_cli_json_and_exit_status(self):
        import json, subprocess, sys
        from support import ROOT

        args = [
            sys.executable,
            str(ROOT / "build.py"),
            str(self.source),
            str(self.output),
            "--language",
            "as2",
            "--compiler",
            str(self.tools[0]),
            "--ir",
            str(self.tools[1]),
            "--linker",
            str(self.tools[2]),
        ]
        result = subprocess.run(args, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)["ok"])
        self.configure(fail="source")
        self.output.write_bytes(b"previous artifact")
        result = subprocess.run(args, capture_output=True, timeout=10)
        self.assertEqual(result.returncode, 1)
        self.rejected(json.loads(result.stdout), "source")
