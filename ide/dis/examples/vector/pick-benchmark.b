implement PickBenchmark;
include "sys.m";
include "math.m";
include "idevector.m";
# Optional model benchmark; no GUI, SVG I/O or fabricated hit implementation.
PickBenchmark: module {
    init: fn(); # Construct a valid imported-style contour, then time real picking.
};


# Compare 20 identical interior clicks on 256 quadratic segments. Initialization
# is outside the interval. Area is nil, so this deliberately tests the geometric
# fallback, not the faster retained brush mask. No latency assertion is made.
init()
{
    sys := load Sys Sys->PATH;
    math := load Math Math->PATH;
    vector := load Idevector Idevector->PATH;
    if (math == nil || vector == nil) {
        sys->print("FAIL: run from the matching installed runtime package\n");
        return;
    }
    doc := vector->create();
    nodes := array[258] of Idevector->Node;
    # New second-control fields are zero for legacy Move/Q/Close commands.
    nodes[0] = Idevector->Node(vector->Move,700,300,0,0, 0, 0);
    for (i := 1; i <= 256; i++) {
        angle := real i*6.283185307179586/256.0;
        middle := (real i-0.5)*6.283185307179586/256.0;
        factor := 1.0/math->cos(3.141592653589793/256.0);
        nodes[i] = Idevector->Node(vector->Quadratic,
            int (500.5+200.0*math->cos(angle)),int (300.5+150.0*math->sin(angle)),
            int (500.5+200.0*factor*math->cos(middle)),int (300.5+150.0*factor*math->sin(middle)), 0, 0);
    }
    nodes[257] = Idevector->Node(vector->Close,0,0,0,0, 0, 0);
    doc.shapes = array[] of {ref Idevector->Shape(16r245ac4,0,1,nodes,nil,0,nil)};
    doc.tool = vector->Select;
    before := sys->millisec();
    for (i = 0; i < 20; i++) {
        vector->pointer(doc,1,500,300);
        vector->pointer(doc,3,500,300);
    }
    elapsed := sys->millisec()-before;
    sys->print("PICK benchmark: 20 interior clicks, 256 quadratics, elapsed=%d ms\n",elapsed);
    # Measure the real Begin separately from overlay/plan serialization. These
    # timings exclude transport, rendering and OS scheduling on purpose.
    doc.tool = vector->Nodes;
    vector->wireparts(doc);
    selectionms := 0;
    presentationms := 0;
    payload := 0;
    for (i = 0; i < 20; i++) {
        start := sys->millisec();
        vector->pointer(doc,1,700,300);
        admitted := sys->millisec();
        parts := vector->wireparts(doc);
        presented := sys->millisec();
        selectionms += admitted-start;
        presentationms += presented-admitted;
        payload = 0;
        for (part := 0; part < len parts; part++) {
            payload += len parts[part];
        }
        vector->pointer(doc,3,700,300);
    }
    sys->print("NODE BEGIN benchmark: 20 calls, selection=%d ms, overlay/plan/wire=%d ms, last payload=%d ASCII bytes; no UI/transport\n",
               selectionms,presentationms,payload);

}
