// AS2 source sketch corresponding to the generated SWF's three separate frames.
// Frame 1: create a named display target and enqueue two independent resource requests.
_root.createEmptyMovieClip("child", 1); // Retain the child's identity and parent display slot.
loadMovie("nested/child.swf", "child"); // Resolve against this startup movie's origin directory.
_root.loadVariables("root-vars.txt"); // Decode form values into the selected root object.
// Frame 2: the native safe point has already fetched and applied root variables.
if (_root.greeting != "hello from root") { // The generator emits a self-checking bytecode branch.
    throw "root variables missing"; // Failed loading must make the example fail visibly.
} // End the root-data check.
// Frame 3: the child's first frame has fetched its own relative variable resource.
if (_root.child.answer != "child origin works") { // Child provenance differs from the root directory.
    throw "child variables missing"; // This also detects accidentally resolving from process CWD.
} // End the child-data check.
stop(); // Keep the scene stable; the host's three-tick limit closes the preview.
