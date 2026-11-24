# Copyright (c) 2026 bhgv
# EN: Verify closed, turning and clipped vector brush sweeps through public editing.
# RU: Проверять замкнутые, поворотные и обрезанные мазки через публичный редактор.

implement IdepaintSweep;
include "sys.m";
include "idevector.m";

sys: Sys;     # Test diagnostics and failure halt.
v: Idevector; # Production SVG editor loaded in the native Dis process.


# EN: Run the brush sweep acceptance fixtures in an isolated Dis application.
# RU: Запустить проверки мазков в изолированном приложении Dis.
IdepaintSweep: module
{
    init: fn(); # VM test entry.
};


# EN: Halt with a named public behavior on failure.
# RU: Остановить тест с названием нарушенного поведения.
require(ok: int, message: string)
{
    if (!ok) {
        sys->print("FAIL brush sweep: %s\n", message);
        fd := sys->open("/dev/sysctl", Sys->OWRITE);
        if (fd != nil) {
            data := array of byte "halt";
            sys->write(fd, data, len data);
        }
        raise "fail: brush sweep";
    }
}


# EN: Open an empty or seeded page with a logical diameter of twenty.
# RU: Открыть пустой или подготовленный лист с логическим диаметром двадцать.
openpage(paths: string): ref Idevector->Document
{
    (status, doc) := v->parse("<svg width=\"1000\" height=\"600\">"+paths+"</svg>");
    require(status == v->Ok, "open sweep page");
    doc.width = 20;
    return doc;
}


# EN: Deliver each authored position through the same deferred brush phases.
# RU: Передать все позиции через штатные фазы отложенного мазка.
draw(doc: ref Idevector->Document, points: array of (int, int))
{
    (x, y) := points[0];
    require(v->defer(doc, 1, x, y) == v->Ok, "start brush");
    for (i := 1; i < len points-1; i++) {
        (x, y) = points[i];
        require(v->defer(doc, 2, x, y) == v->Ok, "capture brush motion");
    }
    (x, y) = points[len points-1];
    require(v->defer(doc, 3, x, y) == v->Ok && v->refine(doc) == v->Ok, "release and refine brush");
}


# EN: Count one published command type across the exact SVG boundary.
# RU: Посчитать команды одного типа в опубликованной границе SVG.
verbs(shape: ref Idevector->Shape, verb: int): int
{
    count := 0;
    for (i := 0; i < len shape.nodes; i++) {
        count += shape.nodes[i].verb == verb;
    }
    return count;
}


# EN: Check a known quarter-arc by the end and first tangent handle.
# RU: Проверить известную четверть окружности по концу и первой ручке.
arc(shape: ref Idevector->Shape, x, y, cx, cy: int): int
{
    for (i := 0; i < len shape.nodes; i++) {
        command := shape.nodes[i];
        if (command.verb == v->Cubic && command.x == x && command.y == y &&
            command.cx == cx && command.cy == cy) {
            return 1;
        }
    }
    return 0;
}


# EN: Pick a logical point without distance tolerance, then cancel selection.
# RU: Выделить в логической точке без допуска и отменить действие.
atpoint(doc: ref Idevector->Document, x, y: int): int
{
    doc.tool = v->Select;
    doc.pickradius = 0.0;
    require(v->pointer(doc, 1, x, y) == v->Ok, "inspect sweep coverage");
    chosen := doc.edit.selected;
    v->cancel(doc);
    return chosen;
}


# EN: Verify that the public SVG reader preserves the complete published path.
# RU: Убедиться, что повторное чтение сохраняет весь опубликованный путь.
roundtrip(doc: ref Idevector->Document)
{
    saved := v->svg(doc);
    (status, copy) := v->parse(saved);
    require(status == v->Ok && v->svg(copy) == saved, "sweep SVG round trip");
}


# EN: A closed rectangle has one outside contour, four exact outside nib
# quarters and a single empty inside. Closing the path shares the first stamp.
# RU: Замкнутый прямоугольник оставляет внешний контур, четыре четверти
# кисти и пустую середину. Начальный отпечаток используется один раз.
closed()
{
    doc := openpage("");
    draw(doc, array[] of {(100, 100), (300, 100), (300, 260), (100, 260), (100, 100)});
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 2 &&
            verbs(doc.shapes[0], v->Cubic) == 4 && arc(doc.shapes[0], 310, 100, 306, 90),
            "closed sweep keeps the original cubic nib and one hole");
    require(atpoint(doc, 200, 100) == 0 && atpoint(doc, 200, 180) == -1, "closed sweep leaves the center empty");
    roundtrip(doc);
    require(v->history(doc, 0) && len doc.shapes == 0, "one undo removes the whole closed stroke");
}


# EN: The horizontal interval is retraced before a perpendicular turn.
# Its far endpoint keeps its exact cap, and the turn remains connected.
# RU: Кисть идёт назад по горизонтали и поворачивает под прямым углом.
# Дальний конец сохраняет точную дугу, а поворот остаётся связанным.
backtrack()
{
    doc := openpage("");
    draw(doc, array[] of {(100, 100), (220, 100), (130, 100), (130, 220)});
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 1 &&
            arc(doc.shapes[0], 230, 100, 226, 90) && arc(doc.shapes[0], 130, 230, 140, 226),
            "reverse and square corner keep both geometric round caps");
    require(atpoint(doc, 200, 100) == 0 && atpoint(doc, 130, 185) == 0 && atpoint(doc, 180, 165) == -1,
            "returning over part of a stroke adds no stray ink");
    roundtrip(doc);
}


# EN: Two oblique centerline intervals cross. Both enclosed lobes remain
# holes, and straight sides avoid reconstructing the outside from pixels.
# RU: Два наклонных интервала пересекаются. Обе доли остаются отверстиями,
# а внешние прямые стороны сохраняют геометрию.
crossed()
{
    doc := openpage("");
    draw(doc, array[] of {(130, 120), (280, 270), (130, 270), (280, 120), (130, 120)});
    require(len doc.shapes == 1 && verbs(doc.shapes[0], v->Move) == 3 &&
            arc(doc.shapes[0], 130, 110, 120, 114), "crossing sweep keeps both lobes and a fixed-nib arc");
    require(atpoint(doc, 205, 195) == 0 && atpoint(doc, 205, 143) == -1 &&
            atpoint(doc, 205, 247) == -1, "ink at crossing, empty in both lobes");
    roundtrip(doc);
}


# EN: Clip a vertical stroke overlapping the left edge of the actual page.
# The exact cap on the other side still uses the original nib handles.
# RU: Обрезать вертикальный мазок по левой границе страницы. Правая дуга
# сохраняет исходные ручки отпечатка.
clipped()
{
    doc := openpage("");
    draw(doc, array[] of {(5, 100), (5, 240)});
    require(len doc.shapes == 1 && arc(doc.shapes[0], 5, 250, 15, 246), "page clip preserves the untouched quarter arc");
    edge := 0;
    for (i := 0; i < len doc.shapes[0].nodes; i++) {
        command := doc.shapes[0].nodes[i];
        require(command.x >= 0 && command.cx >= 0 && command.c2x >= 0, "no geometry outside left page edge");
        edge += command.verb == v->Segment && command.x == 0;
    }
    require(edge > 0 && atpoint(doc, 1, 160) == 0 && atpoint(doc, 25, 160) == -1, "real page clip boundary");
    roundtrip(doc);
}


# EN: A blue annular stroke cuts the red fill but keeps an independent
# central red island. Moving blue exposes a real gap and Undo restores both.
# RU: Синий кольцевой мазок вырезает красную заливку, оставляя внутри
# отдельный красный островок. Перенос и Undo проверяются отдельно.
ringcut()
{
    doc := openpage("<path fill=\"#e04444\" d=\"M 60 70 C 120 45 270 45 350 70 L 350 320 L 60 320 Z\"/>");
    source := doc.shapes[0];
    draw(doc, array[] of {(120, 110), (280, 110), (280, 250), (120, 250), (120, 110)});
    require(len doc.shapes == 3 && arc(doc.shapes[2], 290, 110, 286, 100),
            "annular sweep remains vector and cuts two independent red regions");
    outside := atpoint(doc, 85, 165);
    center := atpoint(doc, 200, 180);
    require(outside >= 0 && center >= 0 && outside != center && doc.shapes[outside].color == 16re04444 &&
            doc.shapes[center].color == 16re04444, "red center visible and separate");
    require(v->pointer(doc, 1, 175, 110) == v->Ok && doc.edit.selected == 2 &&
            v->pointer(doc, 3, 175, 410) == v->Ok && atpoint(doc, 175, 110) == -1 &&
            atpoint(doc, 200, 180) == center, "moving blue leaves an annular gap around red center");
    require(v->history(doc, 0) && v->history(doc, 0) && doc.shapes[0] == source,
            "undo annular stroke and move");
}


# EN: Execute the user-visible outcomes on the real model.
# RU: Проверить пользовательские результаты на рабочей модели.
init()
{
    sys = load Sys Sys->PATH;
    v = load Idevector Idevector->PATH;
    require(v != nil, "load sweep model");
    closed();
    sys->print("PASS brush sweep closed rounded loop\n");
    backtrack();
    sys->print("PASS brush sweep reversal and sharp turn\n");
    crossed();
    sys->print("PASS brush sweep oblique crossing and two holes\n");
    clipped();
    sys->print("PASS brush sweep vector page clip\n");
    ringcut();
    sys->print("PASS brush sweep annular cut, move and Undo\n");
}
