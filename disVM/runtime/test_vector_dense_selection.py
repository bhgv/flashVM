"""Reproduce selection-to-Nodes on a dense closed quadratic contour through the real UI."""
from pathlib import Path
import math
import sys
import tempfile
import xml.etree.ElementTree as ET

from test_desktop import envelope, mouse, run, shortcut
from test_desktop_vector import create_events


# Build a dense filled oval with 256 exact quadratic segments. The independently
# generated source has legal bounded commands; presentation handles are not source.
def svg():
    commands = ["M 700 300"]
    for index in range(256):
        angle = (index+1)*2*math.pi/256
        middle = (index+0.5)*2*math.pi/256
        factor = 1/math.cos(math.pi/256)
        commands.append("Q %d %d %d %d" % (
            round(500+200*math.cos(middle)*factor),round(300+150*math.sin(middle)*factor),
            round(500+200*math.cos(angle)),round(300+150*math.sin(angle))))
    commands.append("Z")
    return '<svg xmlns="http://www.w3.org/2000/svg"><path fill="#245ac4" stroke="none" d="'+" ".join(commands)+'"/></svg>'


# Borrow tested products. Real Open, object selection, Nodes, a contour click and
# Save must not terminate transport or serialize handles into the source document.
def main():
    runtime,module,font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="dense quadratic selection ") as directory:
        work = Path(directory)
        source = work/"dense.svg"
        source.write_text(svg())
        initial = create_events(True)
        views,unused,unused = run(runtime,module,font,mode,work,initial+[envelope(1,65539,1)])
        left,top = list(views.values())[-1][5][1:3]
        scale = min((808-80)/1040,(580-34)/640)
        rows = [3,4,5,6,8,9,10,11,0,1,2,3,4,5,12,7,13,0,1,2]

        # Ordinary physical child-control click, including release admission.
        def button(index):
            return [mouse(round(left+20),round(top+40+28*rows[index])),
                    mouse(round(left+20),round(top+40+28*rows[index]),3)]

        # Logical page point through the actual maximized canvas transform.
        def point(x,y,phase):
            return mouse(round(left+40+(20+x)*scale),round(top+28+(20+y)*scale),phase)

        events = initial+shortcut("O")+[envelope(5,text=str(source)),envelope(4,13,1)]
        events += button(17)+[point(500,300,1),point(500,300,3)]
        events += button(18)+[point(700,300,1),point(700,300,3)]
        events += shortcut("S")+[envelope(1,65539,1)]
        views,unused,capture = run(runtime,module,font,mode,work,events)
        paths = list(ET.fromstring(source.read_text()))
        if len(paths) != 1 or paths[0].attrib["d"].split().count("Q") != 256:
            raise RuntimeError("Node overlay changed the persisted contour")
        if not any("Nodes" in value[5][3] for value in views.values() if 5 in value):
            raise RuntimeError("Dense contour did not reach the Nodes presentation")
        # Reproduce the reported authoring sequence as well as the imported
        # worst-case contour: filled oval plus connected same-colour Brush ink.
        union = work/"oval-brush.svg"
        events = initial+button(2)+button(14)
        events += [point(300,150,1),point(700,450,3)]+button(0)
        events += [point(500,200,1),point(500,80,2),point(600,180,3)]
        events += button(17)+[point(500,300,1),point(500,300,3)]+button(18)
        events += shortcut("S")+[envelope(5,text=str(union)),envelope(4,13,1),envelope(1,65539,1)]
        run(runtime,module,font,mode,work,events)
        if len(list(ET.fromstring(union.read_text()))) != 1:
            raise RuntimeError("Connected same-colour oval/brush union lost its object identity")
    print("PASS dense selection: 256 quadratics, object-to-Nodes, handles, source-only Save",mode)


if __name__ == "__main__":
    main()
