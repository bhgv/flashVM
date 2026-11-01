# Visible authoring status belongs to Limbo, not native drawing policy.
Idevectorlabels: module
{
    PATH: con "idevectorlabels.dis"; # Portable pure label formatter.

    # Borrow scalar tool/width/fill state; return one bounded caption line.
    # No allocation of geometry, callbacks, channels or native handles occurs.
    state: fn(tool, width, fill: int): string;
};
