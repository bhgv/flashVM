"""Edit real figures/nodes through rendered toolbar controls and save only source SVG."""
from pathlib import Path
import re
import sys
import tempfile
import xml.etree.ElementTree as ET

from test_desktop import envelope, mouse, run, shortcut
from test_desktop_vector import create_events


# Borrow actual built products. All tool changes/hits travel through the desktop
# input channel. Compare saved independent objects, without calling model helpers.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="vector selection ") as directory:
        work = Path(directory)
        initial = create_events(True)
        views, unused, capture = run(runtime, module, font, mode, work,
                                     initial + [envelope(1, 65539, 1)])
        left, top = list(views.values())[-1][5][1:3]
        wide, high = 808, 580
        scale = min((wide - 80) / 1040, (high - 34) / 640)
        rows = [3,4,5,6,8,9,10,11,0,1,2,3,4,5,12,7,13,0,1,2]

        # Control and page coordinates are physical positions after maximization.
        def button(index):
            x = round(left + (wide - 20 if 8 <= index < 14 else 20))
            y = round(top + 40 + rows[index] * 28)
            return [mouse(x, y), mouse(x, y, 3)]

        # These are ordinary native pointer events; editing tools must bypass the
        # stamp/primitive capture path and receive their whole admitted gesture.
        def point(x, y, phase):
            return mouse(round(left + 40 + (20 + x) * scale),
                         round(top + 28 + (20 + y) * scale), phase)

        events = initial + button(1) + button(14)
        events += [point(100,100,1), point(300,250,3)]
        events += [point(500,100,1), point(700,250,3)]
        events += button(17) + [point(150,150,1), point(200,210,2), point(200,210,3)]
        events += button(18) + [point(150,160,1), point(160,170,3)]
        events += [envelope(4,17,1), point(350,160,1), point(350,160,3), envelope(4,17,0)]
        events += [point(160,170,1), point(170,180,3)]
        destination = work / "edited.svg"
        events += shortcut("S") + [envelope(5, text=str(destination)), envelope(4,13,1)]
        events += [envelope(1,65539,1)]
        views, unused, capture = run(runtime,module,font,mode,work,events)
        paths = list(ET.fromstring(destination.read_text()))
        if len(paths) != 2:
            raise RuntimeError("selection overlay leaked into SVG or objects were combined")
        first = [int(value) for value in re.findall(r"-?\d+", paths[0].attrib["d"])]
        second = [int(value) for value in re.findall(r"-?\d+", paths[1].attrib["d"])]
        if (abs(first[0]-170)>5 or abs(first[1]-180)>5 or
                abs(first[2]-360)>5 or abs(first[3]-170)>5 or
                abs(second[0]-500)>2 or abs(second[1]-100)>2):
            raise RuntimeError("object/node edit changed wrong coordinates: " + repr((first,second)))
        if not any("Nodes" in value[5][3] for value in views.values() if 5 in value):
            raise RuntimeError("node tool was not activated through its real button")
    print("PASS actual object/node toolbar, independent figures, SVG without overlay", mode)


if __name__ == "__main__":
    main()
