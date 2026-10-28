#include "lib9.h"


/* Link the real public Rune formatter providers from the selected static library.
 * No parameters; return zero on success or one after reporting the failed contract.
 * Allocated strings belong to this caller and are freed before returning. This
 * single-threaded test performs no file I/O except failure diagnostics. */
int main(void)
{
    Rune small[4];
    Rune *owned;
    Rune *flushed;
    Fmt formatter;
    int i;

    small[0] = 123;
    if (runesnprint(small, 0, "%s", "ignored") != -1 || small[0] != 123)
    {
        fprint(2, "zero-capacity Rune formatting changed caller storage\n");
        return 1;
    }
    if (runesnprint(small, 4, "%s", "А😀БВ") != 3 || small[0] != 0x410 || small[1] != 0x1f600 || small[2] != 0x411 ||
        small[3] != 0)
    {
        fprint(2, "bounded Rune wrapper lost scalar indexing or termination\n");
        return 1;
    }
    owned = runesmprint("%4096s%s", "x", "😀");
    if (owned == nil)
    {
        fprint(2, "allocated Rune wrapper failed\n");
        return 1;
    }
    for (i = 0; i < 4095; i++)
    {
        if (owned[i] != ' ')
        {
            free(owned);
            fprint(2, "grown Rune buffer lost its prefix\n");
            return 1;
        }
    }
    if (owned[4095] != 'x' || owned[4096] != 0x1f600 || owned[4097] != 0)
    {
        free(owned);
        fprint(2, "grown Rune buffer lost cursor or Unicode suffix\n");
        return 1;
    }
    free(owned);
    if (runefmtstrinit(&formatter) < 0)
    {
        fprint(2, "Rune formatter initialization failed\n");
        return 1;
    }
    if (fmtprint(&formatter, "%s:%d", "Ж", 42) < 0)
    {
        free(formatter.start);
        fprint(2, "public Rune formatter append failed\n");
        return 1;
    }
    flushed = runefmtstrflush(&formatter);
    if (flushed != formatter.start || flushed == nil || flushed[0] != 0x416 || flushed[1] != ':' || flushed[2] != '4' ||
        flushed[3] != '2' || flushed[4] != 0)
    {
        free(formatter.start);
        fprint(2, "Rune flush lost caller ownership or values\n");
        return 1;
    }
    free(flushed);
    return 0;
}
