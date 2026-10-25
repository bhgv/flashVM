// Explanatory AS2 frame scripts matching the executable generated start.swf; not a compiler input fixture.
// Frame 1: clear the previous drawing and draw the blue square at x=4.
_root.clear();             // Remove geometry from the preceding timeline frame.
_root.beginFill(0x4080ff); // Use an opaque blue fill.
_root.moveTo(4, 4);        // Start the square in stage pixels.
_root.lineTo(12, 4);       // Top edge.
_root.lineTo(12, 12);      // Right edge.
_root.lineTo(4, 12);       // Bottom edge.
_root.lineTo(4, 4);        // Close the outline.
_root.endFill();           // Publish the filled square.
// Frame 2 uses the same commands with x=18 and fill 0xff8040; make_movie.py encodes both frames.
// The host pauses timeline advancement; this script does not implement an instruction debugger.
