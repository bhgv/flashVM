"""Admit locations only when compiler records match the exact captured snapshot."""
import hashlib
import json

from support import BuildCase, driver


# Host-side protocol tests use explicit process doubles, not compiler acceptance.
class TestStructuredDiagnostics(BuildCase):
    # Create the canonical record for an identifier after a Unicode comment/tab.
    def record(self, **updates):
        result = dict(schema=1, kind="semantic", code=0, has_location=True,
                      byte_offset=14, byte_length=7, line=1, column=15,
                      message="unknown name")
        result.update(updates)
        return result

    # Check byte-to-character conversion and provenance through a failed build.
    def test_exact_snapshot_location(self):
        self.source.write_text("/*中*/\ttrace(missing);", encoding="utf-8")
        self.configure(fail="source", error=json.dumps(self.record()))
        result = self.build(structured_diagnostics=True)
        self.rejected(result, "source")
        digest = hashlib.sha256(self.source.read_bytes()).hexdigest()
        item = result["diagnostics"][0]
        self.assertEqual((item["character_offset"], item["character_length"]), (12, 7))
        self.assertEqual((item["line"], item["column"], item["byte_column"]), (1, 13, 15))
        self.assertEqual(item["source_sha256"], digest)
        self.assertEqual(result["source_sha256"], digest)
        self.assertIn("--diagnostics-json", result["steps"][0]["argv"])
        self.assertEqual(item["coordinate_units"], "unicode_characters")

    # Invalid records remain useful raw logs, but must never fabricate navigation.
    def test_malformed_coordinates_stay_unlocated(self):
        data = "/*中*/\ttrace(missing);".encode()
        variants = [self.record(schema=True), self.record(schema=2), self.record(kind="foreign"),
                    self.record(code=True), self.record(code=3), self.record(byte_offset=-1),
                    self.record(byte_offset=999), self.record(byte_length=999),
                    self.record(byte_offset=3), self.record(byte_offset=4, byte_length=1),
                    self.record(line=2), self.record(column=13), self.record(has_location=1),
                    self.record(message=None), self.record(has_location=False),
                    self.record(byte_offset=2147483648)]
        messages = [json.dumps(v) for v in variants] + ["source:3:7: opaque", "{}", "[]", "null",
                    '{"schema":1,"schema":1}',
                    json.dumps(self.record()).replace('"schema": 1', '"schema": 2, "schema": 1'),
                    json.dumps(self.record()) + "noise"]
        for message in messages:
            with self.subTest(message=message):
                item = driver._source_diagnostic(message, data, self.source)
                self.assertNotIn("line", item)
                self.assertNotIn("character_offset", item)
                self.assertEqual(item["message"], message)
        item = driver._source_diagnostic(json.dumps(self.record()), b"\xff" * 30, self.source)
        self.assertNotIn("line", item)

    # EOF spans and physical LF/CRLF/tab coordinates retain exact character units.
    def test_eof_and_multiline(self):
        data = "中\r\n\t😀".encode()
        record = self.record(kind="syntax", code=45, byte_offset=len(data), byte_length=0,
                             line=2, column=6)
        item = driver._source_diagnostic(json.dumps(record), data, self.source)
        self.assertEqual((item["character_offset"], item["character_length"]), (5, 0))
        self.assertEqual((item["line"], item["column"]), (2, 3))

    # Older compilers remain compatible unless the new option is requested.
    def test_opt_in_does_not_change_legacy(self):
        self.configure(fail="source", error="source:3:7: legacy")
        result = self.build()
        self.assertNotIn("--diagnostics-json", result["steps"][0]["argv"])
        self.assertEqual(result["diagnostics"][0]["line"], 3)
        result = self.build(structured_diagnostics=True)
        self.assertNotIn("line", result["diagnostics"][0])
