# Copyright (c) 2026 bhgv
# EN: Verify report precision, repeated entity records and refusal of malformed fields.
# RU: Проверить report precision, repeated entity records и отказ malformed fields.
import unittest
from runtime_stats import parse


# EN/RU: Independent report cases preserve raw identity/counters without claiming live coherence.
# Independent report cases сохраняют raw identity/counters без заявления live coherence.
class ReportTests(unittest.TestCase):

    # EN/RU: Compare owned observations; fail explicitly without Python assert.
    # Сравнить owned observations; explicit failure без Python assert.
    def check_equal(self, actual, expected):
        if actual != expected:
            self.fail(repr(actual) + " != " + repr(expected))

    # EN/RU: Require false observation metadata; fail explicitly on contradictory claims.
    # Требовать false observation metadata; explicit failure при contradictory claims.
    def check_false(self, value):
        if value:
            self.fail("unexpected true observation")

    # EN/RU: Preserve uint64 values and repeated worker identities; ignore application output.
    # Сохранить uint64 values и repeated worker identities; игнорировать application output.
    def test_precision_and_entities(self):
        result = parse(
            "application text\nNATIVEWORKERS_THREAD id=1 turns=18446744073709551615\nNATIVEWORKERS_THREAD id=2 turns=0\nPROXYGC tokens=3\ninstructions=42 gc_runs=0\n"
        )
        self.check_equal(len(result["records"]), 4)
        self.check_equal(result["records"][0]["fields"]["turns"], "18446744073709551615")
        self.check_false(result["coherent_live_snapshot"])

    # EN/RU: Malformed records publish no subset; valid subsequent records remain identified.
    # Malformed records не публикуют subset; valid subsequent records сохраняют identity.
    def test_malformed_record(self):
        result = parse("PROXYGC tokens=1 tokens=2\nNATIVEGC workers=3\n")
        self.check_false(result["complete"])
        self.check_equal(result["errors"][0]["line"], 1)
        self.check_equal([r["tag"] for r in result["records"]], ["NATIVEGC"])

    # EN/RU: Preserve event labels and unfamiliar diagnostic families for future adapters.
    # Сохранить event labels и unfamiliar diagnostic families для будущих adapters.
    def test_events(self):
        result = parse("NATIVEGUEST retired pid=7 thread=99\nPROXYFUTURE owners=4\n")
        self.check_equal(result["records"][0]["labels"], ["retired"])
        self.check_equal(result["records"][1]["tag"], "PROXYFUTURE")


if __name__ == "__main__":
    unittest.main()
