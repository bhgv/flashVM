# Copyright (c) 2026 bhgv
# EN: Verify image selection and the startup bytes of a second Styx executable.
# RU: Проверка выбора образа и стартовых данных второй программы Styx.
implement Namedlaunch;

include "launchcontext.m";

Namedlaunch: module
{
    init: fn();
};


# EN: Accept only the distinct input expected by this provider-side image.
# RU: Принять только входные данные, предусмотренные этим образом владельца.
init()
{
    context := load Launchcontext Launchcontext->PATH;
    if (context == nil || string context->arguments() != "secondary") {
        raise "wrong arguments for the second Styx executable";
    }
}
