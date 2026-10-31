"""Physical input helpers for the documented compact desktop toolbar."""
from test_desktop import mouse


# A complete physical click; no command is sent directly to the Limbo owner.
def click(x, y):
    return [mouse(x, y), mouse(x, y, 3)]


# Use actual shelf/palette button positions at the standard 960x640 replay size.
# This helper changes inputs when a control moves; it never changes runtime hit tests.
def tool(name):
    direct = {"Files": 1, "Save": 2, "Build": 3, "Edit": 4, "Debug": 5, "Help": 6,
              "Search": 7, "Find": 8, "Undo": 9, "Redo": 10, "View": 11}
    if name in direct:
        return click(932, 69 + (direct[name] - 1) * 32)
    groups = {"SVG": ("Files", 4), "Open": ("Files", 5), "SaveAs": ("Files", 7),
              "All": ("View", 0), "Max/N": ("View", 1), "MDI": ("View", 2), "SDI": ("View", 3),
              "Copy": ("Edit", 2), "Cut": ("Edit", 3), "Paste": ("Edit", 4)}
    owner, row = groups[name]
    top = {"Files": 48, "Edit": 144, "View": 368}[owner]
    return tool(owner) + click(766 + (row % 3) * 49, top + 21 + (row // 3) * 32)
