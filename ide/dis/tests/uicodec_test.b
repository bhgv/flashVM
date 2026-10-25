implement UicodecTest;
include "sys.m";
include "uibridge.m";

UicodecTest: module
{
    init: fn(); # Execute real fixed-width codec behavior in the selected Dis ABI.
};


# Verify signed int32 geometry and reject wider NC64 values before wire truncation.
# No parameters; exceptions fail the real runtime test, without native codec substitutes.
init()
{
    sys := load Sys Sys->PATH;
    codec := load Uibridge Uibridge->PATH;
    if (sys == nil || codec == nil) {
        raise "fail: UI codec modules";
    }
    values := array[] of {int (big -2147483648), -3, -1, 0, 1, 3, 2147483647};
    for (i := 0; i < len values; i++) {
        for (j := 0; j < len values; j++) {
            message := ref Uibridge->Message(codec->Pointer, 0, 1, 1, values[i], values[j], "Я😀");
            (encoded, bytes) := codec->encode(message);
            if (!encoded) {
                raise "fail: codec rejected valid signed bounds";
            }
            (decoded, result) := codec->decode(bytes);
            if (!decoded || result.first != values[i] || result.second != values[j] || result.text != "Я😀") {
                raise "fail: codec lost int32 sign or Unicode";
            }
        }
    }
    # The classic ABI cannot represent these wider values; NC64 must reject them before encoding.
    wide := int (big 16r100000000);
    if (wide != 0) {
        message := ref Uibridge->Message(codec->Request, 1, 1, 0, wide, 0, "");
        (ok, bytes) := codec->encode(message);
        if (ok || bytes != nil) {
            raise "fail: codec truncated a wide signed argument";
        }
        message.first = 0;
        message.code = wide;
        (ok, bytes) = codec->encode(message);
        if (ok || bytes != nil) {
            raise "fail: codec truncated a wide operation";
        }
        message.code = 1;
        message.flags = wide;
        (ok, bytes) = codec->encode(message);
        if (ok || bytes != nil) {
            raise "fail: codec truncated wide flags";
        }
    }
    sys->print("PASS UI codec: int32 signed bounds, NC64 width rejection and Unicode values\n");
}
