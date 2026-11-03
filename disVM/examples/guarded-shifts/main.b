implement Shifts;

include "sys.m";

Shifts: module {
    init: fn(); # Entry point; no arguments or returned owned data.
};

# Check six existing native-cell shifts with WORD counts and independent arithmetic.
# Own all scalar locals here. No channels or shared references are published;
# failed checks raise an exception, while Sys print may block in its normal host path.
init()
{
    sys := load Sys Sys->PATH;
    if(sys == nil) {
        raise "cannot load Sys";
    }

    for(i := 0; i < 8; i++) {
        # Counts remain int/WORD even when the shifted value is only one byte.
        n := i;
        scale := 1 << n;
        b := byte 200;
        w := -73;
        l := big -9000000007;

        # Left shifts wrap; unsigned byte and signed word/long right shifts differ.
        bl := b << n;
        br := b >> n;
        wl := w << n;
        wr := w >> n;
        ll := l << n;
        lr := l >> n;

        # Division with ceil of the magnitude models sign-filled negative shifts.
        if(int bl != ((200*scale)&255) || int br != 200/scale ||
            wl != -73*scale || wr != -((73+scale-1)/scale) ||
            ll != big -9000000007 * big scale ||
            lr != -((big 9000000007+big scale-big 1)/big scale))
        {
            raise "shift mismatch";
        }
    }
    sys->print("PASS guarded shifts: six emitted operations\n");
}
