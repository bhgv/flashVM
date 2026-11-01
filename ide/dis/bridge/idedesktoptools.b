# Activate one admitted toolbar release. Windows and Help presentation belong
# to the caller's sole layout process; returned drawer/focus/action are values.
# Never invoke a document command or native UI object from this pure policy call.
toolactivate(windows: array of ref Window, drawer, focus, command: int): (int, int, int)
{
    action := 0;
    if (tools->palette(command)) {
        if (drawer == command) {
            drawer = 0;
        } else {
            drawer = command;
        }
    } else if (command == Help && windows[2] != nil) {
        windows[2].hidden = !windows[2].hidden;
        drawer = 0;
        focus = 3;
        if (windows[2].hidden) {
            focus = 1;
        }
        action = Focus;
    } else {
        drawer = 0;
        action = command;
    }
    return (drawer, focus, action);
}
