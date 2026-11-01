"""Compare batched/legacy wheel distance and returned desktop pixels in real Dis."""
from pathlib import Path
import sys
import tempfile
from test_desktop import run, envelope


# Explicit real runtime/module/font and mode are borrowed. Check both directions,
# multiple rounds, viewport clamping and invalid phases without timing thresholds.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="wheel batches ") as directory:
        work = Path(directory)
        source = work / "source.b"
        source.write_text("# scroll buffer stays unchanged\n" * 30)
        quit_events = [envelope(1, 65539), envelope(1, 65539, 1)]
        baseline, unused, capture = run(runtime, module, font, mode, work, quit_events, source)
        original = capture.read_bytes()
        initial = list(baseline.values())[-1]
        for phase, repeats in ((0, 16), (1, 16), (2, 16), (3, 16), (32, 1)):
            events = []
            for cycle in range(3):
                for code in (5, 4):
                    events += [envelope(3, code, phase, first=700*20, second=500*20)] * repeats
            views, offsets, final = run(runtime, module, font, mode, work, events + quit_events, source)
            if max(offsets) != 1920 or offsets[-1] != 0 or list(views.values())[-1] != initial:
                raise RuntimeError("Wheel batching changed distance, reversal or final geometry")
            returned = final.read_bytes()
            differences = [abs(a-b) for a, b in zip(original, returned) if a != b]
            if len(original) != len(returned) or len(differences) > 192 or max(differences, default=0) > 1:
                raise RuntimeError("Returned scroll raster differs beyond a one-level antialias tolerance")
        for phase in (4, 17, 33):
            views, offsets, final = run(runtime, module, font, mode, work,
                [envelope(3, 5, phase, first=700*20, second=500*20)] + quit_events, source)
            if max(offsets) != 0 or list(views.values())[-1] != initial:
                raise RuntimeError("Malformed wheel count moved the desktop")
    print("PASS actual wheel batches: legacy phases,16-step distance, repeated reversals, returned pixels and refusal", mode)


if __name__ == "__main__":
    main()
