#!/usr/bin/env python3
"""Protect ATG action ordering, literals, nesting and canonical idempotence."""
import unittest
from atg_format import format_grammar, semantic_signature, tokens, ACTION_COLUMN


class AtgFormatTests(unittest.TestCase):
    def grammar(self, rule):
        return (
            '/* fixture */\nCOMPILER X\n#include <string.h>\nCHARACTERS\nletter="a".\nTOKENS\nident=letter {letter}.\nPRODUCTIONS\n'
            + rule
            + "\nEND X.\n"
        )

    def test_actions_end_lines_keep_order_and_align(self):
        source = self.grammar("X=(. int n=0; .) ident (. n++; .) [ ident (. n--; .) ] .")
        actual = format_grammar(source)
        self.assertEqual(semantic_signature(source), semantic_signature(actual))
        self.assertEqual(format_grammar(actual), actual)
        for line in actual.splitlines():
            if "(." in line:
                self.assertEqual(line.index("(."), ACTION_COLUMN)
                self.assertTrue(line.endswith(".)"))
        lines = actual.splitlines()
        for i, line in enumerate(lines):
            if line.strip() == ".":
                self.assertEqual(lines[i + 1], "")

    def test_opaque_c_and_terminal_delimiters(self):
        source = self.grammar('X="(.)" (. puts(".)"); /* .) is not the end */ .) ident .')
        actual = format_grammar(source)
        self.assertEqual(semantic_signature(source), semantic_signature(actual))
        self.assertEqual(
            len([v for k, v in tokens(actual[actual.index("CHARACTERS") :]) if k == "action"]), 1
        )

    def test_nested_groups_have_visible_indentation(self):
        actual = format_grammar(self.grammar('X=ident { [ (ident | "x") ] } .'))
        self.assertIn("    {\n        [\n            (\n", actual)
        self.assertEqual(format_grammar(actual), actual)

    def test_pragma_action_remains_after_its_period(self):
        source = self.grammar("X=ident.").replace(
            "PRODUCTIONS", 'PRAGMAS\np="x". (. count++; .)\nPRODUCTIONS'
        )
        actual = format_grammar(source)
        self.assertEqual(semantic_signature(source), semantic_signature(actual))
        self.assertIn("  .\n\n" + " " * ACTION_COLUMN + "(. count++; .)", actual)

    def test_comments_and_multiline_actions_are_idempotent(self):
        source = self.grammar(
            "/* example\n   continuation */\nX=(.\n    if (x) {\n        f();\n    }\n.) ident ."
        )
        actual = format_grammar(source)
        self.assertEqual(format_grammar(actual), actual)
        self.assertEqual(semantic_signature(source), semantic_signature(actual))

    def test_signature_detects_semantic_reordering(self):
        self.assertNotEqual(
            semantic_signature(self.grammar("X=ident (. f(); .) .")),
            semantic_signature(self.grammar("X=(. f(); .) ident .")),
        )

    def test_malformed_input_is_rejected(self):
        for rule in ["X=(ident .", 'X=(. puts("bad); .) ident .', "X=ident (. missing ."]:
            with self.subTest(rule=rule), self.assertRaises(ValueError):
                format_grammar(self.grammar(rule))


if __name__ == "__main__":
    unittest.main()
