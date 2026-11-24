# Copyright (c) 2026 bhgv
# EN: Sweep a round vector nib along a simple sampled pointer path.
# RU: Провести круглую векторную кисть по простому пути указателя.


# EN: Private simplified centerline. Count is negative if its budget ran out.
# RU: Частная упрощённая осевая линия. Отрицательный count означает отказ.
BrushSpine: adt
{
    points: array of Point; # Owned centerline knots in capture order.
    count: int;             # Initialized knots or -1.
};


# EN: Unit tangent and length of one centerline interval.
# RU: Единичная касательная и длина интервала осевой линии.
BrushLeg: adt
{
    unit: CutPoint; # Tangent in capture order.
    length: real;   # Length in logical page units.
};


# EN: Mutable construction buffer local to one worker.
# RU: Изменяемый буфер построения одного воркера.
BrushOutline: adt
{
    nodes: array of Node; # Owned boundary commands in traversal order.
    count: int;           # Initialized commands.
    radius: real;         # Nib radius in logical page units.
};


# EN: Retain centerline knots within one logical unit of all captured samples.
# For backtracking the nearest point is clamped to the chord. The caller
# installs the initial knot and provides a bounded private output array.
# RU: Сохранить узлы осевой линии с допуском одной единицы листа. При ходе
# назад ближайшая точка ограничена хордой. Первый узел и буфер даёт вызывающий.
brushsimplify(samples: array of Node, first, last, depth: int, spine: ref BrushSpine, work: ref Work)
{
    if (spine.count < 0) {
        return;
    }
    a := cutpoint(samples[first].x, samples[first].y);
    b := cutpoint(samples[last].x, samples[last].y);
    direction := cutsub(b, a);
    length2 := cutdot(direction, direction);
    if (depth == 16 || spine.count == len spine.points) {
        spine.count = -1;
        return;
    }
    maxdist := 1.0;
    split := -1;
    for (i := first+1; i < last; i++) {
        if (i % 128 == 0) {
            checkwork(work);
        }
        p := cutpoint(samples[i].x, samples[i].y);
        t := 0.0;
        if (length2 > 0.0) {
            t = cutdot(cutsub(p, a), direction)/length2;
        }
        if (t < 0.0) {
            t = 0.0;
        }
        if (t > 1.0) {
            t = 1.0;
        }
        delta := cutsub(p, cutoffset(a, direction, t));
        dist := cutdot(delta, delta);
        if (dist > maxdist) {
            split = i;
            maxdist = dist;
        }
    }
    if (split >= 0) {
        brushsimplify(samples, first, split, depth+1, spine, work);
        brushsimplify(samples, split, last, depth+1, spine, work);
        return;
    }
    point := Point(samples[last].x, samples[last].y);
    if (!brushequal(spine.points[spine.count-1], point)) {
        spine.points[spine.count++] = point;
    }
}


# EN: Compute unit tangents of a borrowed forward-moving centerline. A leg
# whose projection is too small along the whole gesture uses mask tracing.
# RU: Вычислить касательные прямого по ходу пути. Слишком малая проекция
# сегмента на общее направление отправляет путь на обводку маски.
brushlegs(path: array of Point, math: Math): array of BrushLeg
{
    end := path[len path-1];
    axis := cutpoint(end.x-path[0].x, end.y-path[0].y);
    axis2 := cutdot(axis, axis);
    legs := array[len path-1] of BrushLeg;
    for (i := 0; i < len legs; i++) {
        delta := cutpoint(path[i+1].x-path[i].x, path[i+1].y-path[i].y);
        length2 := cutdot(delta, delta);
        forward := cutdot(delta, axis);
        if (forward <= 0.0 || forward*forward < 0.08*length2*axis2) {
            return nil;
        }
        length := math->sqrt(length2);
        legs[i] = BrushLeg(CutPoint(delta.x/length, delta.y/length), length);
    }
    return legs;
}


# EN: Get the directed unit tangent for a borrowed leg on either pass.
# RU: Получить направленную касательную сегмента при любом проходе.
brushdirection(legs: array of BrushLeg, index, step: int): BrushLeg
{
    if (step > 0) {
        return legs[index];
    }
    leg := legs[index-1];
    leg.unit = CutPoint(-leg.unit.x, -leg.unit.y);
    return leg;
}


# EN: Write an offset side point unless its rounded coordinate is already
# present. Only this local builder owns and updates the output.
# RU: Записать точку боковой границы, если её округлённые координаты новые.
# Только сборщик меняет свой буфер.
brushwall(out: ref BrushOutline, p: CutPoint)
{
    x := int p.x;
    y := int p.y;
    if (out.count == 0) {
        out.nodes[out.count++] = Node(Move, x, y, 0, 0, 0, 0);
    } else {
        node := out.nodes[out.count-1];
        if (node.x != x || node.y != y) {
            out.nodes[out.count++] = Node(Segment, x, y, 0, 0, 0, 0);
        }
    }
}


# EN: Join the two tangents at a centerline knot. Outside joins keep a cubic
# circular arc with tangent handles. Inside joins intersect the two offsets.
# Refuse corners that would consume too much of either neighbouring leg.
# RU: Пройти поворот осевой линии. Внешний стык получает кубическую дугу
# с касательными ручками. Внутренний стык пересекает смещения соседних
# частей. Повороты, занимающие слишком большую часть соседей, отклоняются.
brushangle(out: ref BrushOutline, center: CutPoint, before, after: BrushLeg, math: Math): int
{
    direction := cutcross(before.unit, after.unit);
    cosine := cutdot(before.unit, after.unit);
    if (cosine < 0.25) {
        return 0;
    }
    radius := out.radius;
    n := CutPoint(before.unit.y, -before.unit.x);
    m := CutPoint(after.unit.y, -after.unit.x);
    if (direction <= 0.0) {
        trim := -radius*direction/(1.0+cosine);
        if (3.0*trim >= before.length || 3.0*trim >= after.length) {
            return 0;
        }
        mid := cutlerp(n, m, 0.5);
        brushwall(out, cutoffset(center, mid, 2.0*radius/(1.0+cosine)));
        return 1;
    }
    a := cutoffset(center, n, radius);
    d := cutoffset(center, m, radius);
    brushwall(out, a);
    if (int a.x != int d.x || int a.y != int d.y) {
        halfcos := math->sqrt((1.0+cosine)*0.5);
        handle := (2.0/3.0)*radius*direction/(halfcos*(1.0+halfcos));
        b := cutoffset(a, before.unit, handle);
        c := cutoffset(d, after.unit, -handle);
        out.nodes[out.count++] = cutnode(CutCurve(Cubic, a, b, c, d), d);
    }
    return 1;
}


# EN: Construct one side of the nib and a two-quarter-arc terminal cap.
# The second pass reverses the centerline so its outer joins remain clockwise.
# RU: Построить одну сторону кисти с двумя четвертями окружности на конце.
# Обратный проход меняет ход осевой линии, сохраняя обход по часовой стрелке.
brushside(out: ref BrushOutline, path: array of Point, legs: array of BrushLeg,
    first, last, step: int, math: Math): int
{
    leg := brushdirection(legs, first, step);
    p := path[first];
    brushwall(out, cutoffset(cutpoint(p.x, p.y), CutPoint(leg.unit.y, -leg.unit.x), out.radius));
    for (i := first+step; i != last; i += step) {
        next := brushdirection(legs, i, step);
        p = path[i];
        if (!brushangle(out, cutpoint(p.x, p.y), leg, next, math)) {
            return 0;
        }
        leg = next;
    }
    p = path[last];
    tx := out.radius*leg.unit.x;
    ty := out.radius*leg.unit.y;
    brushwall(out, CutPoint(real p.x+ty, real p.y-tx));
    out.nodes[out.count++] = brusharc(p, ty, -tx, tx, ty);
    out.nodes[out.count++] = brusharc(p, tx, ty, -ty, tx);
    return 1;
}


# EN: Prepare the short boundary of a forward sweep with ample space for
# its corner offsets. This retains the established gentle-turn anchors.
# RU: Построить короткую границу прямого по ходу мазка с местом для смещений.
# Сохраняются привычные узлы плавных поворотов.
brushforward(path: array of Point, width: int, math: Math, work: ref Work): array of Node
{
    if (len path == 2) {
        return brushround(path[0], path[1], width);
    }
    legs := brushlegs(path, math);
    if (legs == nil) {
        return nil;
    }
    out := ref BrushOutline(array[4*len path+6] of Node, 0, real width/2.0);
    if (!brushside(out, path, legs, 0, len path-1, 1, math) ||
        !brushside(out, path, legs, len path-1, 0, -1, math)) {
        return nil;
    }
    out.nodes[out.count++] = Node(Close, 0, 0, 0, 0, 0, 0);
    nodes := out.nodes[0:out.count];
    for (i := 0; i < len nodes; i++) {
        node := nodes[i];
        if (!pointok(node.x, node.y) || !pointok(node.cx, node.cy) || !pointok(node.c2x, node.c2y)) {
            return nil;
        }
    }
    edges := cutcurves(nodes);
    search := ref CutSearch(array[1] of CutHit, 0, 5000, work);
    if (edges == nil || !cutsimple(edges, search)) {
        return nil;
    }
    return nodes;
}


# EN: Build an editable swept boundary for a copied raw stroke. Keep the
# compact forward construction when possible, then use the fixed-nib graph.
# RU: Построить редактируемую границу сырого мазка. Для простого прохода
# сохранить короткое построение, иначе использовать граф круглого отпечатка.
brushsweep(raw: ref Shape, work: ref Work): array of Node
{
    samples := raw.nodes;
    if (raw.width < 2 || len samples < 2 || samples[len samples-1].verb != Segment) {
        return nil;
    }
    spine := ref BrushSpine(array[24] of Point, 1);
    spine.points[0] = Point(samples[0].x, samples[0].y);
    brushsimplify(samples, 0, len samples-1, 0, spine, work);
    if (spine.count < 2) {
        return nil;
    }
    path := spine.points[0:spine.count];
    bound := (raw.width+1)/2;
    clipped := 0;
    for (i := 0; i < len path; i++) {
        p := path[i];
        if (!pointok(p.x-bound, p.y-bound) || !pointok(p.x+bound, p.y+bound)) {
            clipped = 1;
            break;
        }
    }
    math := load Math Math->PATH;
    if (math == nil) {
        return nil;
    }
    if (!clipped && raw.width >= 4) {
        nodes := brushforward(path, raw.width, math, work);
        if (nodes != nil) {
            return nodes;
        }
    }
    return brushunion(path, raw.width, clipped, math, work);
}
