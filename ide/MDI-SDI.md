# MDI/SDI on one MovieClip desktop

[Русский](MDI-SDI.ru.md) · [Vector drawing](VECTOR-EDITOR.md) · [Debugger](DEBUGGER.md)

There is one scrolling IDE desktop. Source editors, graphic documents and debug
panels remain MovieClip windows on that desktop. There are no separate Source,
Graphics or Debug workspaces/tabs replacing the desktop.

A future visible MDI container will group ordinary document windows. Source and
graphics containers must be different because their tools differ. More than one
container of either family may exist alongside SDI windows. Detaching a child
restores SDI presentation without reopening a file, changing its identity or
losing edits. Moving/zooming a container must compose its children's transforms;
selection/input must still use exact document/view revisions.

## Implemented foundation

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

## Remaining graphical integration

The visible shell still displays SDI panes. Container MovieClip parenting,
container toolbars, child geometry/clipping, tab/cascade commands and interactive
join/detach gestures are **not implemented** by this registry. No UI button
pretends to perform grouping. The registry is tested application infrastructure,
not evidence that a visual MDI container already exists.

The next adapter must preserve saved SDI geometry, reject parenting cycles,
revalidate stale input after reparenting, retain pending asynchronous colour jobs
by document identity/revision, and detach all children when dissolving a container.
A source tab must never inherit brush controls; a graphic tab must never receive
source-editor compiler shortcuts as a result of focus alone.
