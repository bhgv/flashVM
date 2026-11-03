implement MappedCopy;

include "sys.m";

MappedCopy: module
{
    init: fn();
};

# The String is a managed reference; the integers are ordinary native values.
# A record assignment therefore needs MOVMP ownership handling, not raw memmove.
Record: adt
{
    name: string;
    number: int;
    checksum: int;
};


# Compile with the hosted native-cell compiler (-F). The VM chooses the ordinary
# or experimental guarded implementation at startup; source/bytecode are the same.
# This example does not enable parallel managed heap execution or new Dis64 calls.
init()
{
    sys := load Sys Sys->PATH;
    records := array[2] of Record;

    # The source record keeps its own name reference. Every assignment acquires
    # another count for the destination before replacing its previous owner.
    records[0] = Record("mapped owner", 0, 731);
    for (index := 0; index < 1000; index++)
    {
        records[0].number = index;
        records[1] = records[0];
    }

    # Both the complete byte image and its managed String must agree. Checks stay
    # explicit, so an optimized build cannot silently omit the observations.
    if (records[1].name != "mapped owner" || records[1].number != 999 ||
        records[1].checksum != 731)
    {
        raise "mapped replacement lost a field or owner";
    }

    # Replacing the destination String releases its old owner. The source record
    # still owns its original String, independently of the destination lifetime.
    records[1].name = nil;
    if (records[0].name != "mapped owner")
    {
        raise "destination release changed source ownership";
    }
    sys->print("PASS guarded mapped example: 1000 copies, source intact\n");
}
