"""Use real zoom controls, wheel/pinch routing, marquee, reset and unchanged SVG."""
from pathlib import Path
import sys
import tempfile

from test_desktop import envelope, mouse, run, shortcut
from test_desktop_vector import create_events


# Read camera values from the actual native receiver's admitted scene, not a mock.
def camera(work):
    rows = [line.split() for line in (work / "trace.txt").read_text().splitlines()
            if line.startswith("VECTORCAMERA ") and line.split()[2] == "5"]
    if not rows:
        raise RuntimeError("No admitted native camera")
    return tuple(map(int, rows[-1][3:]))


# Borrow runtime/module/font products. Replay ordinary control/pointer messages;
# compare actual native camera, captured clipping and persisted source bytes.
def main():
    runtime,module,font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="canvas viewport ") as directory:
        work = Path(directory)
        initial = create_events(True)
        views,unused,capture = run(runtime,module,font,mode,work,initial+[envelope(1,65539,1)])
        left,top = list(views.values())[-1][5][1:3]
        scale = min((808-80)/1040,(580-34)/640)
        rows = [3,4,5,6,8,9,10,11,0,1,2,3,4,5,12,7,13,0,1,2]

        # Real child control bounds; all command semantics remain on the owner.
        def button(index):
            x = round(left+(788 if 8 <= index < 14 else 20))
            y = round(top+40+28*rows[index])
            return [mouse(x,y),mouse(x,y,3)]

        # Coordinates are original page units before each fresh replay's zoom.
        def point(x,y,phase):
            return mouse(round(left+40+(20+x)*scale),round(top+28+(20+y)*scale),phase)

        drawing = initial+button(1)+button(14)+[point(100,100,1),point(400,300,3)]
        zooming = drawing+button(19)
        unused,unused,capture = run(runtime,module,font,mode,work,zooming+[envelope(1,65539,1)])
        baseline = capture.read_bytes().split(b"\n",3)[3]
        if camera(work) != (100,500,300):
            raise RuntimeError("Activating zoom modified camera/source")
        focusx = round(left+40+270*scale)
        focusy = round(top+28+220*scale)
        wheel = envelope(3,4,1,first=focusx*20,second=focusy*20)
        pinch = envelope(3,8,1,first=focusx*20,second=focusy*20)
        run(runtime,module,font,mode,work,zooming+[wheel,pinch,envelope(1,65539,1)])
        if camera(work)[0] != 156:
            raise RuntimeError("Wheel/pinch did not reach independent canvas camera")
        destination = work/"view.svg"
        selection = zooming+[point(200,100,1),point(450,250,2),point(450,250,3)]
        saving = shortcut("S")+[envelope(5,text=str(destination)),envelope(4,13,1)]
        unused,unused,capture = run(runtime,module,font,mode,work,selection+saving+[envelope(1,65539,1)])
        zoom,x,y = camera(work)
        if not (395 <= zoom <= 410 and abs(x-325)<=2 and abs(y-175)<=2):
            raise RuntimeError("Marquee did not fit/centre selection: "+repr((zoom,x,y)))
        source = destination.read_bytes()
        pixels = capture.read_bytes().split(b"\n",3)[3]
        # The fixed gutter is outside the page viewport and contains no toolbar.
        for py in range(round(top+70),round(top+400)):
            for px in range(round(left+36),round(left+40+20*scale)-1):
                at = (py*960+px)*3
                if pixels[at:at+3] != baseline[at:at+3]:
                    raise RuntimeError("Zoomed geometry escaped the page viewport")
        reset = selection+[envelope(3,12,1,first=round(left+20)*20,second=round(top+40+56)*20)]
        run(runtime,module,font,mode,work,reset+saving+[envelope(1,65539,1)])
        if camera(work) != (100,500,300) or destination.read_bytes() != source:
            raise RuntimeError("Double zoom-button reset changed source or failed 1:1")
    print("PASS canvas viewport: wheel, pinch, marquee, clipping, double reset and unchanged SVG",mode)


if __name__ == "__main__":
    main()
