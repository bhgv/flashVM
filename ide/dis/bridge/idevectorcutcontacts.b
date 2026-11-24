# Copyright (c) 2026 bhgv
# EN: Find exact straight seams and isolate bounded, supported contour contacts.
# RU: Находить общие прямые кромки и поддерживаемые касания контуров.


# EN: One common interval, parameterized on each directed original edge.
# RU: Общий участок с параметрами на обоих направленных исходных сегментах.
CutSeam: adt
{
    first: int;    # First source edge ID.
    second: int;   # Second source edge ID.
    firstlo: real; # Start parameter on first edge.
    firsthi: real; # End parameter on first edge.
    otherlo: real; # Smaller parameter on second edge.
    otherhi: real; # Larger parameter on second edge.
    same: int;     # Nonzero when the inside sides point the same way.
};


# EN: Private overlap table for one boolean, sharing its bounded hit search.
# RU: Частная таблица общих участков одной операции с общим поиском событий.
CutContacts: adt
{
    search: ref CutSearch; # Owned crossing and contact table.
    seams: array of CutSeam; # Owned shared spans, initialized up to count.
    count: int;              # Number of stored shared spans.
    math: Math;              # Available pure scalar math operations.
};


# EN: Clamp a known near-endpoint parameter to the closed unit interval.
# RU: Ограничить параметр около концов замкнутым единичным отрезком.
cutunit(value: real): real
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}


# EN: Compare two intersection positions before rounding to an SVG grid point.
# RU: Сравнить две позиции до округления к координате сетки SVG.
cutsamepoint(p, q: CutPoint): int
{
    d := cutsub(p, q);
    return cutdot(d, d) < 1.0e-12;
}


# EN: Record the end contacts and oriented interior of a shared span.
# RU: Записать касания на концах и направление общего участка.
cutseam(pair: CutPair, lo, hi, u, v: real, contacts: ref CutContacts)
{
    (a, ignored) := cuteval(pair.a, lo);
    cuthitadd(contacts.search, pair, lo, u, a);
    if (contacts.search.count < 0 || hi <= lo+0.00000001) {
        return;
    }
    (b, unused) := cuteval(pair.a, hi);
    cuthitadd(contacts.search, pair, hi, v, b);
    if (contacts.count == len contacts.seams) {
        contacts.search.count = -1;
        return;
    }
    same := u < v;
    if (!same) {
        (u, v) = (v, u);
    }
    contacts.seams[contacts.count++] = CutSeam(pair.first, pair.second, lo, hi, u, v, same);
}


# EN: Intersect two straight segments, including endpoint and overlap events.
# Bounding boxes and parameter checks exclude their unbounded supporting lines.
# RU: Пересечь два отрезка, включая общие концы и наложение. Прямоугольники
# и диапазоны параметров ограничивают их опорные прямые.
cutlinelines(pair: CutPair, contacts: ref CutContacts)
{
    a := cutsub(pair.a.d, pair.a.a);
    b := cutsub(pair.b.d, pair.b.a);
    delta := cutsub(pair.b.a, pair.a.a);
    cross := cutcross(a, b);
    if (cross != 0.0) {
        t := cutcross(delta, b)/cross;
        u := cutcross(delta, a)/cross;
        if (t >= -0.00000001 && t <= 1.00000001 && u >= -0.00000001 && u <= 1.00000001) {
            t = cutunit(t);
            u = cutunit(u);
            cuthitadd(contacts.search, pair, t, u, cutoffset(pair.a.a, a, t));
        }
        return;
    }
    if (cutcross(delta, a) != 0.0) {
        return;
    }
    alen := cutdot(a, a);
    blen := cutdot(b, b);
    lo := cutdot(delta, a)/alen;
    hi := cutdot(cutsub(pair.b.d, pair.a.a), a)/alen;
    if (lo > hi) {
        (lo, hi) = (hi, lo);
    }
    if (hi < 0.0 || lo > 1.0) {
        return;
    }
    lo = cutunit(lo);
    hi = cutunit(hi);
    u := cutunit(cutdot(cutsub(cutoffset(pair.a.a, a, lo), pair.b.a), b)/blen);
    v := cutunit(cutdot(cutsub(cutoffset(pair.a.a, a, hi), pair.b.a), b)/blen);
    cutseam(pair, lo, hi, u, v, contacts);
}


# EN: Evaluate a scalar cubic in power basis. Coefficients are ordered
# from constant to cubic and describe signed distance from a line.
# RU: Вычислить скалярный кубический полином. Коэффициенты от нулевой
# до третьей степени описывают знаковое расстояние до прямой.
cutpoly(c: array of real, t: real): real
{
    return ((c[3]*t+c[2])*t+c[1])*t+c[0];
}


# EN: Split the cubic into intervals monotone between derivative roots.
# Internal breakpoints include tangency candidates.
# RU: Разделить кубический полином на монотонные отрезки по корням
# производной. Внутренние точки включают возможные касания.
cutderivative(c: array of real, math: Math): array of real
{
    a := 3.0*c[3];
    b := 2.0*c[2];
    d := c[1];
    bounds := array[4] of real;
    bounds[0] = 0.0;
    count := 1;
    if (a > -1.0e-9 && a < 1.0e-9) {
        if (b < -1.0e-9 || b > 1.0e-9) {
            t := -d/b;
            if (t > 0.0 && t < 1.0) {
                bounds[count++] = t;
            }
        }
    } else {
        delta := b*b-4.0*a*d;
        if (delta >= 0.0) {
            root := math->sqrt(delta);
            left := (-b-root)/(2.0*a);
            right := (-b+root)/(2.0*a);
            if (left > right) {
                (left, right) = (right, left);
            }
            if (left > 0.0 && left < 1.0) {
                bounds[count++] = left;
            }
            if (right > 0.0 && right < 1.0 && right > left+0.00000001) {
                bounds[count++] = right;
            }
        }
    }
    bounds[count++] = 1.0;
    return bounds[0:count];
}


# EN: Record a root on the curve only when its position is on the segment.
# Keep event parameters in the pair's original order.
# RU: Записать корень на кривой, если его позиция принадлежит отрезку.
# Порядок параметров совпадает с исходной парой.
cutlinehit(pair: CutPair, a, b: CutCurve, direction: CutPoint, t: real, swapped: int, search: ref CutSearch)
{
    (p, ignored) := cuteval(b, t);
    u := cutdot(cutsub(p, a.a), direction)/cutdot(direction, direction);
    if (u < -0.00000001 || u > 1.00000001) {
        return;
    }
    u = cutunit(u);
    if (swapped) {
        cuthitadd(search, pair, t, u, p);
    } else {
        cuthitadd(search, pair, u, t, p);
    }
}


# EN: Intersect a bounded line and a Bézier from the roots of their signed
# distance. Repeated roots and endpoint hits are retained as contacts.
# RU: Пересечь отрезок и Безье по корням знакового расстояния. Кратные
# корни и совпадения с концами сохраняются как касания.
cutlinecurve(pair: CutPair, line, curve: CutCurve, swapped: int, contacts: ref CutContacts)
{
    direction := cutsub(line.d, line.a);
    f0 := cutcross(direction, cutsub(curve.a, line.a));
    f1 := cutcross(direction, cutsub(curve.b, line.a));
    f2 := cutcross(direction, cutsub(curve.c, line.a));
    f3 := cutcross(direction, cutsub(curve.d, line.a));
    tolerance := 1.0e-7*contacts.math->sqrt(cutdot(direction, direction));
    if (f0*f0 <= tolerance*tolerance && f1*f1 <= tolerance*tolerance &&
        f2*f2 <= tolerance*tolerance && f3*f3 <= tolerance*tolerance) {
        contacts.search.count = -1;
        return;
    }
    c := array[] of {f0, 3.0*(f1-f0), 3.0*(f2-2.0*f1+f0), f3-3.0*f2+3.0*f1-f0};
    bounds := cutderivative(c, contacts.math);
    previous := cutpoly(c, bounds[0]);
    if (previous*previous <= tolerance*tolerance) {
        cutlinehit(pair, line, curve, direction, 0.0, swapped, contacts.search);
    }
    for (i := 1; i < len bounds && contacts.search.count >= 0; i++) {
        value := cutpoly(c, bounds[i]);
        if (value*value <= tolerance*tolerance) {
            cutlinehit(pair, line, curve, direction, bounds[i], swapped, contacts.search);
        } else if (previous*previous > tolerance*tolerance && (value > 0.0) != (previous > 0.0)) {
            lo := bounds[i-1];
            hi := bounds[i];
            for (j := 0; j < 44; j++) {
                t := (lo+hi)*0.5;
                if ((cutpoly(c, t) > 0.0) == (previous > 0.0)) {
                    lo = t;
                } else {
                    hi = t;
                }
            }
            cutlinehit(pair, line, curve, direction, (lo+hi)*0.5, swapped, contacts.search);
        }
        previous = value;
    }
}


# EN: Compare directed cubic geometry before clipping. A line and a curve
# use the polynomial route unless they are both actual straight segments.
# RU: Сравнить направленную геометрию кубики до обрезки. Отрезок и кривая
# обрабатываются по полиному, кроме пары обычных отрезков.
cutsamecurve(a, b: CutCurve): int
{
    return cutsamepoint(a.a, b.a) && cutsamepoint(a.b, b.b) &&
        cutsamepoint(a.c, b.c) && cutsamepoint(a.d, b.d);
}


# EN: When non-flat control hulls just touch along an axis, Bernstein weights
# exclude interior contacts on that axis. Check only the four endpoint pairs.
# RU: Если неплоские управляющие оболочки соприкасаются по оси, веса
# Бернштейна исключают внутренние касания по ней. Проверить пары концов.
cutaxiscontacts(pair: CutPair, search: ref CutSearch)
{
    a := array[] of {pair.a.a, pair.a.d};
    b := array[] of {pair.b.a, pair.b.d};
    for (i := 0; i < len a && search.count >= 0; i++) {
        for (j := 0; j < len b && search.count >= 0; j++) {
            if (cutsamepoint(a[i], b[j])) {
                cuthitadd(search, pair, real i, real j, a[i]);
            }
        }
    }
}


# EN: Dispatch supported pairs of edges from different filled outlines.
# General free-form curve intersections retain the bounded hull solver.
# RU: Разобрать поддерживаемые пары ребер разных заливок. Для общих
# пересечений кривых сохраняется ограниченный поиск по оболочкам.
cutcontactpair(pair: CutPair, contacts: ref CutContacts)
{
    search := contacts.search;
    if (search.count < 0 || search.budget <= 0) {
        search.count = -1;
        return;
    }
    if (search.budget % 64 == 0) {
        checkwork(search.work);
    }
    search.budget--;
    a := cutbounds(pair.a);
    b := cutbounds(pair.b);
    if (cutapart(a, b)) {
        return;
    }
    if (pair.a.verb == Segment && pair.b.verb == Segment) {
        cutlinelines(pair, contacts);
    } else if (pair.a.verb == Segment) {
        cutlinecurve(pair, pair.a, pair.b, 0, contacts);
    } else if (pair.b.verb == Segment) {
        cutlinecurve(pair, pair.b, pair.a, 1, contacts);
    } else if (cutsamecurve(pair.a, pair.b)) {
        cutseam(pair, 0.0, 1.0, 0.0, 1.0, contacts);
    } else if (cutsamecurve(pair.a, cutreverse(pair.b))) {
        cutseam(pair, 0.0, 1.0, 1.0, 0.0, contacts);
    } else if (((a.right == b.left || b.right == a.left) && a.left < a.right && b.left < b.right) ||
        ((a.bottom == b.top || b.bottom == a.top) && a.top < a.bottom && b.top < b.bottom)) {
        cutaxiscontacts(pair, search);
    } else {
        cutpair(pair.a, pair.b, pair.first, pair.second, search);
    }
}
