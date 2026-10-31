implement VectorExample; # Compile/run this ordinary module on our matching DisVM.
include "sys.m"; # The runtime owns output and checked native file primitives.
include "idevector.m"; # The same model used by each independent vector pane.
include "idefileops.m"; # Complete writes/reads without sharing descriptors with UI.

VectorExample: module # A source-built portable entry, not a native drawing application.
{ # Keep the executable interface separate from the reusable vector library.
    init: fn(); # Run once from a fresh working directory to protect existing files.
}; # No mutable application state is exported.


# Create one filled brush contour and a filled ellipse, export without overwriting,
# read that actual file and require an exact canonical SVG roundtrip. All model
# references belong to this single Dis process; no UI/frame dependency is needed.
init()
{ # Local ownership lasts until the example's final output.
    sys := load Sys Sys->PATH; # Resolve the actual runtime's system boundary.
    vector := load Idevector Idevector->PATH; # Resolve the installed model bytecode.
    files := load Idefileops Idefileops->PATH; # Resolve the reusable checked file library.
    if (sys == nil || vector == nil || files == nil) { # Missing dependencies are real failures.
        raise "fail: vector example dependencies"; # Never print a fabricated successful artifact.
    } # All following operations use these matching modules.
    doc := vector->create(); # Start clean, with private bounded Undo/Redo history.
    vector->pointer(doc, 1, 100, 120); # Begin the blue brush stroke at a logical point.
    vector->pointer(doc, 2, 250, 250); # Add a sample; the preview remains one path.
    vector->pointer(doc, 2, 450, 140); # Add a raw sample; no fitting occurs during motion.
    vector->pointer(doc, 3, 700, 260); # Release: commit one Undo unit and synchronously vectorize this library example.
    doc.tool = vector->Ellipse; # Choose the next authoring tool inside this owner.
    doc.color = 16re04444; # Use the same red RGB24 for stroke and fill.
    doc.fill = 1; # Closed figures may have an interior fill.
    vector->pointer(doc, 1, 220, 320); # The first corner bounds the ellipse.
    vector->pointer(doc, 3, 520, 520); # The opposite corner commits its eight quadratic arcs.
    text := vector->svg(doc); # Serialize generic path commands on the1000x600 page.
    status := files->writebytes(sys, "vector-example.svg", array of byte text, 1); # OEXCL protects existing paths.
    if (status != files->Ok) { # Do not mark the document saved after a refused/failed write.
        raise "fail: vector-example.svg exists or could not be written"; # Use a fresh directory next time.
    } # A real file now contains the exact serialized snapshot.
    (readstatus, source) := files->read(sys, "vector-example.svg", Idevector->MaxSVG); # Reopen through the actual file boundary.
    (parsed, copy) := vector->parse(source); # Parse into a new independent document transactionally.
    if (readstatus != files->Ok || parsed != vector->Ok || vector->svg(copy) != text) { # Check bytes and geometry.
        raise "fail: vector example SVG roundtrip"; # A partial/unsupported import is never called successful.
    } # The original document and new parsed document remain separate owners' values.
    sys->print("PASS vector example: vector-example.svg, two paths, exact SVG roundtrip\n"); # Report only verified output.
} # Local references retire normally; the checked file helpers closed their own descriptors.
