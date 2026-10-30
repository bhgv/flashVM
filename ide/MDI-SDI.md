# MDI/SDI on one MovieClip desktop

[Русский](MDI-SDI.ru.md) · [Vector drawing](VECTOR-EDITOR.md) · [Debugger](DEBUGGER.md)

There is one scrolling IDE desktop. Source editors, graphic documents and debug
panels remain MovieClip windows on that desktop. There are no separate Source,
Graphics or Debug workspaces/tabs replacing the desktop.

Visible MDI containers group ordinary document windows. Source and
graphics containers are different because their tools differ. More than one
container of either family may exist alongside SDI windows. Detaching a child
restores SDI presentation without reopening a file, changing its identity or
losing edits. Moving a container composes its children's transforms;
selection/input must still use exact document/view revisions.

## Membership and graphical integration

The portable [Idemdi interface](dis/bridge/idemdi.m) and
[owner](dis/bridge/idemdi.b) maintain bounded document identities, homogeneous
container tool families and document membership through typed channels.
The actual desktop registers source/vector owners and retires their identities
before a slot is reused. Document processes and files remain their original
owners; the registry never receives native pointers or file capabilities.

| Command | Effect |
| --- | --- |
| Register | Admit one unused source/graphics document identity as SDI |
| Create | Admit one unused container identity with a fixed tool family |
| Join | Assign an existing matching-family document to a container |
| Leave | Restore SDI membership, preserving document identity |
| Remove | Retire one document identity; does not shut its owner down |
| Dissolve | Restore every child to SDI and retire only the container |
| Snapshot | Return independent copied family/membership arrays |
| Shutdown | Acknowledge then terminate only the registry |

Mutations require the exact registry revision. A stale or cross-family command
changes nothing. Idempotent membership commands do not advance the revision.
Returned arrays cannot mutate registry state. Identities and containers each
have capacity8 in this first owner. No child reference crosses Dis/Flash heaps.

## Use on the shared desktop

- **MDI** on the right, or **Ctrl+G**, joins the selected source/vector document
  to the first container with the same tool family; otherwise creates one.
- **Shift+MDI** / **Ctrl+Shift+G** creates a separate container of that family.
- **SDI** / **Ctrl+D** detaches just the selected document and restores its saved
  ordinary geometry. Edits, history, file path and document identity stay alive.
- Drag the container's exposed title strip to move it and all children together.
  Another window over that strip owns its clicks normally.
- Thumbnails still identify documents. Selecting one reveals the whole container;
  **All** includes container bounds. Closing the last child retires an empty group.

These are actual MovieClip parent/child relations, not a drawn imitation or
workspace switch. Native presentation subtrees may be recreated when reparented;
the original Dis document process and pending revision-bound colour work remain.
The receiver validates a bounded depth-two tree before committing a scene:
root under table, document under root; each root precedes its children. No parent
pointer or arbitrary/cyclic graph crosses the channel. Native world transforms
compose the document, container and desktop matrices for drawing and hit tests.

## First-version limits

Children currently tile in one/two columns. Individual child move/resize/maximize
inside MDI, tabs/cascade, container resize/docking and shared container toolbars
are pending. Use SDI for ordinary free window gestures. Global desktop zoom works
for the entire hierarchy. Each graphic child retains its real brush/style controls;
source children retain text/compiler controls. Fixed children never change their
saved SDI geometry in response to unsupported child zoom. There are8 desktop pane
slots shared with system/debug windows, and8 bounded container identities.
