implement RealConversion;
include "sys.m";

RealConversion: module
{
    init: fn();
};


# Exercise four numeric conversions emitted by this Limbo compiler.
# No arguments or borrowed objects; retain Sys through this module invocation.
# Load failure or a wrong result raises an ordinary Limbo exception.
init()
{
    sys := load Sys Sys->PATH;
    if (sys == nil)
    {
        raise "load Sys";
    }
    # Dynamic array reads keep the following operations from being folded away.
    words := array[] of {3, 7};
    bigs := array[] of {big 3, big 7};
    reals := array[] of {2.5, 3.5};
    word := words[0];
    bigvalue := bigs[0];
    fraction := reals[0];
    # Integer-to-real conversions use the existing native FP environment.
    fromword := real word;
    frombig := real bigvalue;
    # Limbo rounds a finite real by adding/subtracting .5 and truncating.
    # Consequently 2.5 becomes 3. NaN/out-of-range values raise invalid argument.
    toword := int fraction;
    tobig := big fraction;
    if (fromword != 3.0 || frombig != 3.0 || toword != 3 || tobig != big 3)
    {
        raise "conversion result";
    }
    sys->print("converted: %g %g %d %bd\n", fromword, frombig, toword, tobig);
}
