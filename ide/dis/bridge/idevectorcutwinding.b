# Copyright (c) 2026 bhgv
# EN: Classify the two sides of a source edge with nonzero winding.
# RU: Классифицировать обе стороны сегмента по правилу ненулевого обхода.


# EN: Project a copied point onto the ray axis and its oriented normal.
# RU: Спроецировать точку на луч и его ориентированную нормаль.
cutraypoint(point, origin, ray: CutPoint): CutPoint
{
    delta := cutsub(point, origin);
    return CutPoint(cutdot(delta, ray), cutcross(ray, delta));
}


# EN: Copy controls into coordinates whose positive X axis points to the
# right of the fragment. The change of basis preserves signed winding.
# RU: Перевести ручки в координаты, где +X направлен вправо от участка.
# Замена базиса сохраняет знак обхода.
cutraycurve(edge: CutCurve, origin, ray: CutPoint): CutCurve
{
    return CutCurve(edge.verb, cutraypoint(edge.a, origin, ray), cutraypoint(edge.b, origin, ray),
        cutraypoint(edge.c, origin, ray), cutraypoint(edge.d, origin, ray));
}


# EN: Treat numerical zero on the ray consistently at shared endpoints.
# RU: Одинаково распознать численный ноль у общих концов на луче.
cutrayzero(value: real): real
{
    if (value > -1.0e-10 && value < 1.0e-10) {
        return 0.0;
    }
    return value;
}


# EN: Return the half-open signed crossing at a monotone interval.
# RU: Вернуть знак пересечения полуоткрытого монотонного участка.
cutraysign(a, b: real): int
{
    return (a <= 0.0 && b > 0.0)-(b <= 0.0 && a > 0.0);
}


# EN: Isolate the zero on a monotone interval with a half-open crossing.
# RU: Уточнить ноль монотонного участка с полуоткрытым пересечением.
cutrayroot(poly: array of real, lo, hi, a, b: real): real
{
    if (a == 0.0) {
        return lo;
    }
    if (b == 0.0) {
        return hi;
    }
    for (i := 0; i < 43; i++) {
        t := (hi+lo)*0.5;
        if ((cutpoly(poly, t) > 0.0) == (a > 0.0)) {
            lo = t;
        } else {
            hi = t;
        }
    }
    return (lo+hi)*0.5;
}


# EN: Return winding beyond the origin, the signed origin jump, and the
# number of origin crossings. A negative count denotes ambiguous geometry.
# RU: Вернуть обход за началом луча, знаковый скачок в начале и число
# пересечений в начале. Отрицательное число означает неоднозначность.
cutraycounts(edge: CutCurve, math: Math): (int, int, int)
{
    box := cutbounds(edge);
    band := 1.0e-6;
    if (box.top > 1.0e-10 || box.bottom < -1.0e-10 || box.right < -band) {
        return (0, 0, 0);
    }
    a := cutrayzero(edge.a.y);
    b := cutrayzero(edge.b.y);
    c := cutrayzero(edge.c.y);
    d := cutrayzero(edge.d.y);
    if (box.left > band) {
        # Beyond the origin all signed interior extrema cancel in pairs.
        # Дальше начала знаки внутренних экстремумов взаимно сокращаются.
        return (cutraysign(a, d), 0, 0);
    }
    if (a == 0.0 && b == 0.0 && c == 0.0 && d == 0.0) {
        return (0, 0, -1);
    }
    if (edge.verb == Segment) {
        sign := cutraysign(a, d);
        if (sign == 0) {
            return (0, 0, 0);
        }
        x := cutlerp(edge.a, edge.d, -a/(d-a)).x;
        if (x > band) {
            return (sign, 0, 0);
        }
        if (x >= -band) {
            return (0, sign, 1);
        }
        return (0, 0, 0);
    }
    poly := array[] of {a, 3.0*(b-a), 3.0*(c-2.0*b+a), d-3.0*c+3.0*b-a};
    cuts := cutderivative(poly, math);
    far := 0;
    jump := 0;
    hits := 0;
    before := a;
    for (i := 1; i < len cuts; i++) {
        after := cutrayzero(cutpoly(poly, cuts[i]));
        sign := cutraysign(before, after);
        if (sign != 0) {
            t := cutrayroot(poly, cuts[i-1], cuts[i], before, after);
            (point, ignored) := cuteval(edge, t);
            if (point.x > band) {
                far += sign;
            } else if (point.x >= -band) {
                jump += sign;
                hits++;
            }
        }
        before = after;
    }
    return (far, jump, hits);
}


# EN: Classify the sides against one raw source. Only the active arc and
# certified coincident arcs may account for the winding jump at the origin.
# Refuse a missing jump or an unrecorded near-origin crossing.
# RU: Классифицировать обе стороны относительно сложной заливки. Скачок
# в начале луча дают только текущая дуга и подтверждённые общие дуги.
# Пропавший скачок или неизвестное пересечение отклоняют операцию.
cutrawsides(source: ref CutOutline, start, active: int, point, tangent: CutPoint, t: real,
    contacts: ref CutContacts): (int, int, int)
{
    length2 := cutdot(tangent, tangent);
    if (length2 < 1.0e-12) {
        return (0, 0, 0);
    }
    math := contacts.math;
    scale := 1.0/math->sqrt(length2);
    ray := CutPoint(-tangent.y*scale, tangent.x*scale);
    right := 0;
    change := 0;
    for (i := 0; i < len source.edges; i++) {
        if (i % 16 == 0) {
            checkwork(contacts.search.work);
        }
        id := start+i;
        expected := -1;
        if (id != active) {
            expected = -cutseamside(contacts, active, id, id+1, t);
        }
        (far, jump, hits) := cutraycounts(cutraycurve(source.edges[i], point, ray), math);
        if (jump != expected || hits != (expected != 0)) {
            return (0, 0, 0);
        }
        right += far;
        change += jump;
    }
    return (right != 0, right+change != 0, 1);
}
