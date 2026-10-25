# Dis-owned bounded logical-line viewport; rendering geometry stays in FlashVM.
Ideviewport: module
{
    PATH: con "ideviewport.dis"; # Relative module resource, not an OS window handle.
    Rows: con 24;               # Maximum logical lines in one presentation window.
    Bytes: con 4096;            # Maximum complete UTF-8 bytes, within TextField limits.

    State: adt
    {
        rows: int;  # Actual renderer line capacity admitted by Dis, bounded 1..128.
        first: int; # Unicode scalar offset at the top; may enter a long line to reveal its caret.
    };

    # Return a fresh exclusive viewport at document offset zero; no I/O or channels.
    create: fn(): ref State;

    # Move by bounded logical LF lines, preserving the document and cursor.
    # state is exclusive; text is an acknowledged snapshot, delta is clamped +/-128.
    scroll: fn(state: ref State, text: string, delta: int);

    # Return (first, last, top_line) in Unicode scalars / zero-based logical lines.
    # state is exclusive; text/caret are acknowledged values. reveal follows the caret.
    # At most Bytes UTF-8 bytes are selected, without splitting a scalar; no renderer/OS call.
    window: fn(state: ref State, text: string, caret: int, reveal: int): (int, int, int);
};
