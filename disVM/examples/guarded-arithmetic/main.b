implement Arithmetic;

include "sys.m";

Arithmetic: module
{
    init: fn();
};


# Exercise the fifteen integer operations with one native-cell image.
# No parameters/returned owners; print on success or raise on a bad result.
# Guarded startup captures both operands before storing even overlapping output.
init()
{
    sys := load Sys Sys->PATH;
    for (index := 0; index < 32; index++)
    {
        # byte: two runtime inputs prevent constant-folding the operation.
        leftB := byte (index+64);
        rightB := byte (index+3);
        # sum: compare against the independently typed int expression.
        sumB := leftB + rightB;
        if (int sumB != ((index+64) + (index+3)))
        {
            raise "byte sum changed";
        }
        # difference: compare against the independently typed int expression.
        differenceB := leftB - rightB;
        if (int differenceB != ((index+64) - (index+3)))
        {
            raise "byte difference changed";
        }
        # both: compare against the independently typed int expression.
        bothB := leftB & rightB;
        if (int bothB != ((index+64) & (index+3)))
        {
            raise "byte both changed";
        }
        # either: compare against the independently typed int expression.
        eitherB := leftB | rightB;
        if (int eitherB != ((index+64) | (index+3)))
        {
            raise "byte either changed";
        }
        # different: compare against the independently typed int expression.
        differentB := leftB ^ rightB;
        if (int differentB != ((index+64) ^ (index+3)))
        {
            raise "byte different changed";
        }

        # int: two runtime inputs prevent constant-folding the operation.
        leftW := int (index+64);
        rightW := int (index+3);
        # sum: compare against the independently typed int expression.
        sumW := leftW + rightW;
        if (int sumW != ((index+64) + (index+3)))
        {
            raise "int sum changed";
        }
        # difference: compare against the independently typed int expression.
        differenceW := leftW - rightW;
        if (int differenceW != ((index+64) - (index+3)))
        {
            raise "int difference changed";
        }
        # both: compare against the independently typed int expression.
        bothW := leftW & rightW;
        if (int bothW != ((index+64) & (index+3)))
        {
            raise "int both changed";
        }
        # either: compare against the independently typed int expression.
        eitherW := leftW | rightW;
        if (int eitherW != ((index+64) | (index+3)))
        {
            raise "int either changed";
        }
        # different: compare against the independently typed int expression.
        differentW := leftW ^ rightW;
        if (int differentW != ((index+64) ^ (index+3)))
        {
            raise "int different changed";
        }

        # big: two runtime inputs prevent constant-folding the operation.
        leftL := big (index+64);
        rightL := big (index+3);
        # sum: compare against the independently typed int expression.
        sumL := leftL + rightL;
        if (int sumL != ((index+64) + (index+3)))
        {
            raise "big sum changed";
        }
        # difference: compare against the independently typed int expression.
        differenceL := leftL - rightL;
        if (int differenceL != ((index+64) - (index+3)))
        {
            raise "big difference changed";
        }
        # both: compare against the independently typed int expression.
        bothL := leftL & rightL;
        if (int bothL != ((index+64) & (index+3)))
        {
            raise "big both changed";
        }
        # either: compare against the independently typed int expression.
        eitherL := leftL | rightL;
        if (int eitherL != ((index+64) | (index+3)))
        {
            raise "big either changed";
        }
        # different: compare against the independently typed int expression.
        differentL := leftL ^ rightL;
        if (int differentL != ((index+64) ^ (index+3)))
        {
            raise "big different changed";
        }

    }
    sys->print("PASS guarded arithmetic: 15 operations, 32 input pairs\n");
}
