// ActionScript 2 (DESIGN EXAMPLE, NOT EXECUTABLE YET).
// A future native gc object will expose the same policy as the C API:
// gc.mode("suspended");
// ... temporary object-intensive work ...
// gc.mode("full"); // one pass; previous mode stays suspended
// gc.mode(0);      // resume normal automatic safe-point checks
// trace(gc.state());
