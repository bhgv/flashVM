"""Replay the real Dis browser controller and verify FlashVM pixels and native files."""
from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run

BROWSE = 65564  # Directory path request; first=0, second=view ticket, flags=revision.
MOVE = 65565  # Selected entry step; ticket/revision bind the current browser view.
NEXT = 65566  # Advance one native batch, preserving document state.
CHOOSE = 65567  # Navigate or load the selected file under dirty-source admission.
CLOSE = 65568  # Retire the directory cursor and return to source input.
QUIT = 65539  # Existing dirty-checked/explicit-discard application shutdown.


# Open the human path field, replace its selected prefill and accept via real key input.
# path is borrowed immutable text; returns owned encoded events, no direct widget access.
def browse_path(path):
    return shortcut('B') + [envelope(5, text=str(path)), envelope(4, 13, 1), envelope(4, 13, 0)]


# Execute bounded native fixtures. CLI supplies graphical runtime, module and mode.
# Each replay uses actual file owners, dirty policy, channel transport and raster output.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    with tempfile.TemporaryDirectory(prefix='Graphical directory browser ') as temporary:
        work = Path(temporary)
        source = work / 'source.b'
        source.write_text('seed')
        project = work / 'project Я😀'
        project.mkdir()
        chosen = project / 'chosen Я😀.b'
        chosen.write_text('selected α😀')
        panels = []
        events = browse_path(project) + [envelope(5, text='not inserted'), envelope(4, 46, 1)]
        events += shortcut('V') + [envelope(4, 13, 1), envelope(4, 13, 0)]
        events += shortcut('Z') + [envelope(5, text='X')] + shortcut('S') + [envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source, panels=panels)
        if not any(kind == 9 and 'chosen Я😀.b' in text for kind, _, text in panels):
            raise RuntimeError('browser did not present actual native entry metadata')
        if chosen.read_text() != 'Xselected α😀' or source.read_text() != 'seed':
            raise RuntimeError('selected path adoption/save changed the wrong file')
        if any(view[-1] not in ('seed', 'selected α😀', 'Xselected α😀') for view in views):
            raise RuntimeError('readonly browser forwarded text/paste/delete to the document')
        rendered = (work / 'view.ppm').read_bytes()
        # Capture a browser view independently and require real raster change.
        run(runtime, module, None, mode, work, [envelope(1, BROWSE, 1, str(project), second=1),
                                             envelope(1, QUIT)], source)
        browser_pixels = (work / 'view.ppm').read_bytes()
        run(runtime, module, None, mode, work, [envelope(1, QUIT)], source)
        if browser_pixels == (work / 'view.ppm').read_bytes() or not rendered:
            raise RuntimeError('browser was logged without actual glyph presentation')
        # Stale completed-view tickets must not move/open another selection.
        events = [envelope(1, BROWSE, 1, str(project), second=1),
                  envelope(1, MOVE, 1, first=1, second=1),
                  envelope(1, CHOOSE, 1, second=3), envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source)
        if views[2][1] != 1 or views[-1][-1] != 'Xselected α😀':
            raise RuntimeError('browser ticket guard or actual file open failed: ' + repr(views))
        # Browsing may continue while dirty, but selecting a file cannot discard edits.
        events = [envelope(5, text='dirty'), envelope(1, BROWSE, 2, str(project), second=2),
                  envelope(1, CHOOSE, 2, second=3), envelope(4, 27, 1), envelope(1, QUIT, 1)]
        views = run(runtime, module, None, mode, work, events, source)
        if views[3][1] != 1 or views[-1][-1] != 'dirtyseed':
            raise RuntimeError('browser bypassed unsaved-document admission')
        # Candidate failure preserves the previous selection and actual path.
        events = [envelope(1, BROWSE, 1, str(project), second=1),
                  envelope(1, BROWSE, 1, str(project / 'missing'), second=2),
                  envelope(1, CHOOSE, 1, second=3), envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source)
        if views[2][1] != 4 or views[-1][-1] != 'Xselected α😀':
            raise RuntimeError('failed replacement destroyed the previous browser state')
        nested = work / 'nested'
        (nested / 'folder').mkdir(parents=True)
        (nested / 'folder' / 'inside.b').write_text('inside')
        events = [envelope(1, BROWSE, 1, str(nested), second=1),
                  envelope(1, CHOOSE, 1, second=2), envelope(1, CHOOSE, 1, second=3),
                  envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source)
        if views[-1][-1] != 'inside':
            raise RuntimeError('directory navigation did not resolve a complete native path')
        # Two real native entries remain visible at once. Derive click geometry from
        # the actual raster surface/font extent rather than guessing the row height.
        mouse = work / 'mouse'
        mouse.mkdir()
        (mouse / 'alpha.b').write_text('alpha')
        (mouse / 'beta.b').write_text('beta')
        panels = []
        prefix = [envelope(1, BROWSE, 1, str(mouse), second=1)]
        run(runtime, module, None, mode, work, prefix + [envelope(1, QUIT)], source, panels=panels)
        listing = [text for kind, _, text in panels if kind == 9][-1]
        rows = listing.split('\n')
        if len(rows) != 5 or not all('FILE  ' in row for row in rows[2:4]):
            raise RuntimeError('browser did not display both native rows at once')
        raster = (work / 'view.ppm').read_bytes().split(b'\n', 3)[3]
        tops = [y for y in range(640) if raster[y*960*3:y*960*3+3] == bytes((232, 237, 247))]
        if not tops:
            raise RuntimeError('AS-rendered browser panel is missing')
        top = min(tops)
        step = (640-top-12)/5
        click_y = round((top+8+3.5*step)*20)
        # Raw native coordinates are mapped by completed FlashVM glyph geometry,
        # then ticket-checked by Dis. A row press selects, never opens/edits directly.
        events = prefix + [envelope(3, 1, 1, first=40*20, second=click_y),
                           envelope(3, 1, 3, first=40*20, second=click_y)]
        panels = []
        run(runtime, module, None, mode, work, events + [envelope(1, QUIT)], source, panels=panels)
        if not any(kind == 9 and 'page 1 [2/2]' in text for kind, _, text in panels):
            raise RuntimeError('real pointer row selection did not reach Limbo')
        # Click the actual Open glyphs. Monospaced advance comes from the selected
        # row's real highlight rectangle, so this also detects broken AS text placement.
        raster = (work / 'view.ppm').read_bytes().split(b'\n', 3)[3]
        highlighted = [offset//3 % 960 for offset in range(0, len(raster), 3)
                       if raster[offset:offset+3] == bytes((197, 217, 252))]
        if not highlighted:
            raise RuntimeError('selected row has no actual highlight pixels')
        left, right = min(highlighted), max(highlighted)+1
        advance = (right-left)/len(rows[3])
        open_x = round((left+(rows[1].index('[Open]')+2)*advance)*20)
        open_y = round((top+8+1.5*step)*20)
        events += [envelope(3, 1, 1, first=open_x, second=open_y), envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source)
        expected = rows[3][6:]
        if views[-1][-1] != (mouse / expected).read_text():
            raise RuntimeError('native mouse Open did not adopt the selected file')
        # Parent navigation changes directory ownership while leaving source history intact.
        panels = []
        events = prefix + [envelope(4, 8, 1), envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source, panels=panels)
        if views[-1][-1] != 'seed' or not any(kind == 9 and 'DIR   mouse' in text for kind, _, text in panels):
            raise RuntimeError('parent navigation lost the source or native sibling listing')
        many = work / 'many'
        many.mkdir()
        for index in range(40):
            (many / ('item%02d.b' % index)).write_text('x')
        panels = []
        events = browse_path(many) + [envelope(4, 40, 1), envelope(4, 40, 1), envelope(4, 34, 1)]
        events += [envelope(4, 40, 1)] * 9 + [envelope(4, 34, 1), envelope(4, 82, 1),
                                            envelope(4, 27, 1), envelope(1, QUIT)]
        views = run(runtime, module, None, mode, work, events, source, panels=panels)
        labels = [text for kind, _, text in panels if kind == 9]
        for text in ('page 1 [3/32]', 'page 2 [1/8]', 'page 2 [8/8]', '[end]'):
            if not any(text in label for label in labels):
                raise RuntimeError('missing actual browser paging state: ' + text)
        if any(view[-1] != 'seed' or view[0] != 1 for view in views):
            raise RuntimeError('paging/refresh/cancel modified document history')
    print('PASS graphical browser: real entries, dirty guard, tickets, paging, path adoption and raster,', mode)


if __name__ == '__main__':
    main()
