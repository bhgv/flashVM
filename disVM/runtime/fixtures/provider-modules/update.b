# Copyright (c) 2026 bhgv
# EN: Private bootstrap helper checks old/fresh modules across a live provider update with identical metadata.
# RU: Private bootstrap-helper проверяет old/fresh modules при live-обновлении provider с одинаковыми metadata.


# EN/RU: Borrow caller's ordinary Sys/app module handles, switch future opens and preserve retained old code.
# Заимствовать обычные Sys/app handles caller, переключить будущие opens и сохранить удержанный old-code.
update(sys: Sys, app: Value)
{
    old := load Value "/dep.dis";
    if (old == nil || old->value() != 111) {
        raise "missing old dependency";
    }
    control := sys->open("/switch", Sys->OWRITE);
    if (control == nil || sys->write(control, array of byte "x", 1) != 1) {
        raise "switch refused";
    }
    fresh := load Value "/dep.dis";
    if (fresh == nil || fresh->value() != 112 || old->value() != 111 || app->value() != 112) {
        raise "provider path cache alias or old module mutation";
    }
}
