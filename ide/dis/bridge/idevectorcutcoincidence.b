# Copyright (c) 2026 bhgv
# EN: Certify partial coincidence of polynomial Bézier spans before clipping.
# RU: Подтверждать совпадение частей полиномиальных Безье перед обрезкой.


# EN: Return whether the four controls progress along an oriented projection.
# Common monotone projection excludes an unrelated crossing of the two tails.
# RU: Проверить порядок управляющих точек на общей проекции. Монотонность
# исключает отдельное пересечение концов совпавших кривых.
cutprojection(edge: CutCurve, axis: CutPoint): int
{
    return cutdot(cutsub(edge.b, edge.a), axis) >= 0.0 &&
        cutdot(cutsub(edge.c, edge.b), axis) >= 0.0 &&
        cutdot(cutsub(edge.d, edge.c), axis) >= 0.0;
}


# EN: Recognize a straight span even when its original verb is Q or C.
# RU: Распознать прямой участок, даже если исходная команда была Q или C.
cutstraight(edge: CutCurve): int
{
    if (edge.verb == Segment) {
        return 1;
    }
    axis := cutsub(edge.d, edge.a);
    b := cutcross(axis, cutsub(edge.b, edge.a));
    c := cutcross(axis, cutsub(edge.c, edge.a));
    tolerance := 1.0e-14*cutdot(axis, axis);
    return b*b <= tolerance && c*c <= tolerance;
}


# EN: Locate a point on a borrowed monotone Bézier span by chord projection.
# A negative result means its full two-dimensional position did not match.
# RU: Найти точку на монотонном сегменте по проекции на хорду. Отрицательный
# результат означает, что полная двумерная позиция не совпала.
cutlocate(edge: CutCurve, p: CutPoint): real
{
    if (cutsamepoint(edge.a, p)) {
        return 0.0;
    }
    if (cutsamepoint(edge.d, p)) {
        return 1.0;
    }
    axis := cutsub(edge.d, edge.a);
    target := cutdot(cutsub(p, edge.a), axis);
    if (target <= 0.0 || target >= cutdot(axis, axis)) {
        return -1.0;
    }
    lo := 0.0;
    hi := 1.0;
    for (i := 0; i < 46; i++) {
        t := (lo+hi)*0.5;
        (q, ignored) := cuteval(edge, t);
        if (cutdot(cutsub(q, edge.a), axis) < target) {
            lo = t;
        } else {
            hi = t;
        }
    }
    t := (lo+hi)*0.5;
    (q, ignored) := cuteval(edge, t);
    if (!cutsamepoint(p, q)) {
        return -1.0;
    }
    return t;
}


# EN: Convert distance fraction to the parameter of an encoded straight span.
# A nonuniform quadratic or cubic keeps its original parameterization.
# RU: Перевести долю длины в параметр прямого участка. Неравномерные Q и C
# сохраняют исходную параметризацию.
cutlineparameter(edge: CutCurve, distance: real): real
{
    if (edge.verb == Segment) {
        return distance;
    }
    return cutlocate(edge, cutlerp(edge.a, edge.d, distance));
}


# EN: Add the mapped endpoints of edge a that lie on b to private pairs.
# Stored parameters stay ordered as in the original CutPair.
# RU: Добавить концы ребра a, лежащие на b, в частный список параметров.
# Порядок параметров соответствует исходной паре.
cutendpoints(a, b: CutCurve, reverse: int, matches: array of (real, real), used: int): int
{
    hull := cutbounds(b);
    endpoints := array[] of {a.a, a.d};
    for (i := 0; i < len endpoints; i++) {
        p := endpoints[i];
        if (p.x < hull.left || p.x > hull.right || p.y < hull.top || p.y > hull.bottom) {
            continue;
        }
        u := cutlocate(b, p);
        if (u >= 0.0) {
            if (reverse) {
                matches[used++] = (u, real i);
            } else {
                matches[used++] = (real i, u);
            }
        }
    }
    return used;
}


# EN: Certify a partial seam by comparing the restricted control polygons.
# Equal polynomials share an affine parameter interval. A common monotone
# axis also proves that the unmatched tails cannot cross each other.
# RU: Подтвердить общую часть сравнением ограниченных управляющих
# многоугольников. Совпавшие полиномы имеют общий аффинный интервал, а общая
# монотонная ось исключает пересечения вне него.
cutpartial(pair: CutPair, contacts: ref CutContacts): int
{
    matches := array[4] of (real, real);
    used := cutendpoints(pair.a, pair.b, 0, matches, 0);
    used = cutendpoints(pair.b, pair.a, 1, matches, used);
    if (used < 2) {
        return 0;
    }
    first := matches[0];
    last := first;
    for (i := 1; i < used; i++) {
        if (matches[i].t0 < first.t0) {
            first = matches[i];
        }
        if (matches[i].t0 > last.t0) {
            last = matches[i];
        }
    }
    (lo, u) := first;
    (hi, v) := last;
    if (hi-lo <= 1.0e-8 || (u-v)*(u-v) <= 1.0e-16) {
        return 0;
    }
    other := pair.b;
    if (u > v) {
        other = cutreverse(other);
    }
    axis := cutsub(pair.a.d, pair.a.a);
    if (!cutprojection(other, axis)) {
        return 0;
    }
    part: CutCurve;
    if (u > v) {
        part = cutreverse(cutslice(pair.b, v, u));
    } else {
        part = cutslice(pair.b, u, v);
    }
    if (!cutsamecurve(cutslice(pair.a, lo, hi), part)) {
        return 0;
    }
    cutseam(pair, lo, hi, u, v, contacts);
    return 1;
}
