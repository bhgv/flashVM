"""Check that the experimental build view preserves process snapshot fields."""

import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "native_view", Path(__file__).resolve().parents[1] / "host/prepare_native_registers.py"
)
view = importlib.util.module_from_spec(spec)
spec.loader.exec_module(view)


# Register isolation must not rename saved p->R members or text/comment data.
class NativeViewTests(unittest.TestCase):
    def test_qualified_members_and_literals(self):
        """Only the active interpreter register file is redirected to TLS."""
        source = "R.s = p->R.s; snapshot.R = R; /* R */ x=\"R\"; y='R';\n"
        self.assertEqual(
            view.transform(source),
            "DVM_R.s = p->R.s; snapshot.R = DVM_R; /* R */ x=\"R\"; y='R';\n",
        )

    def test_comments_between_member_tokens(self):
        """Comments do not turn a qualified process register into a global one."""
        self.assertEqual(
            view.transform("p -> /* note */ R.d = R.d;"), "p -> /* note */ R.d = DVM_R.d;"
        )

    def test_dispatch_boundary_requires_review(self):
        """A changed upstream dispatch loop must not silently omit the safepoint."""
        source = "\t\t/* Successful translated steps own their register/PC publication. */"
        output = view.dispatch_safepoint(source)
        self.assertIn("dvm_native_context_poll()", output)
        self.assertLess(output.index("dvm_native_context_poll"), output.index("Successful translated"))
        with self.assertRaises(ValueError):
            view.dispatch_safepoint("unrecognized upstream dispatch")


if __name__ == "__main__":
    unittest.main()
