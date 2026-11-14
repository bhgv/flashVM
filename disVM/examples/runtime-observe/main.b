# Copyright (c) 2026 bhgv
# EN: CLI reader for live resident metadata; no target execution or heap dereference.
# RU: CLI reader live resident metadata; без target execution либо heap dereference.
implement RuntimeObserve;
include "sys.m";
include "processctl.m";
RuntimeObserve: module { init: fn(); };


# EN/RU: Print copied cards from this VM until end; refuse unavailable data explicitly.
# Вывести copied cards этой VM до end; явно отказать unavailable data.
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    sys->print("NATIVEOBSERVER pid=%d scope=residents\n", control->pid());
    names := array[] of {"starting", "dispatch", "scheduler", "wait-work", "teardown", "done"};
    activities := array[] of {"unknown", "ready", "send", "receive", "alt", "native",
                              "table", "child", "debug", "exiting", "broken"};
    cursor := big 0;
    for (;;) {
        (status, id, pid, done, phase) := control->residentstate(cursor);
        if (status == 0) {
            break;
        }
        if (status != 1 || id <= cursor || pid <= 0 || phase < 0 || phase >= len names) {
            raise "live resident observation unavailable";
        }
        (activitystatus, activity) := control->residentactivity(id);
        label := "unavailable";
        if (activitystatus == 0) {
            label = "gone";
        }
        if (activitystatus == 1 && activity >= 0 && activity < len activities) {
            label = activities[activity];
        }
        sys->print("NATIVERESIDENT id=%bd pid=%d done=%d phase_id=%d phase=%s activity_status=%d activity_id=%d activity=%s\n",
                   id, pid, done, phase, names[phase], activitystatus, activity, label);
        cursor = id;
    }
}
