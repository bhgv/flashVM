# Copyright (c) 2026 bhgv
# EN: Construct rounded brush contours in page coordinates for straight strokes.
# RU: Строить скруглённые контуры прямых мазков в координатах листа.


# EN: Join two perpendicular radius vectors by one cubic quarter arc. The
# coordinates are in-page after the caller's bounds check. Return a copied node.
# RU: Соединить два перпендикулярных радиуса кубической четвертью окружности.
# Координаты проверены вызывающим кодом. Вернуть скопированный узел.
brusharc(center: Point, ax, ay, bx, by: real): Node
{
    k := 0.5522847498307936;
    cx := real center.x;
    cy := real center.y;
    return Node(Cubic, int (cx+bx), int (cy+by),
        int (cx+ax+k*bx), int (cy+ay+k*by),
        int (cx+bx+k*ax), int (cy+by+k*ay));
}


# EN: Return a closed capsule or four-arc dot for a borrowed straight stroke.
# Collinear reversals extend the same capsule. Return nil for turns, a nib
# below integer curve resolution, or page clipping. No mutable state is shared.
# RU: Вернуть замкнутую капсулу или точку из четырёх дуг для прямого мазка.
# Обратное движение вдоль линии расширяет ту же капсулу. Повороты, слишком
# мелкие отпечатки и обрезка по листу дают nil. Общее состояние отсутствует.
brushcapsule(stroke: ref Shape, work: ref Work): array of Node
{
    if (stroke.width < 2 || len stroke.nodes < 2) {
        return nil;
    }
    path := stroke.nodes;
    first := Point(path[0].x, path[0].y);
    dx := path[1].x - first.x;
    dy := path[1].y - first.y;
    left := first;
    right := first;
    lo := 0;
    hi := 0;
    for (i := 1; i < len path; i++) {
        if (i % 128 == 0) {
            checkwork(work);
        }
        if (path[i].verb != Segment) {
            return nil;
        }
        x := path[i].x - first.x;
        y := path[i].y - first.y;
        if (x*dy != y*dx) {
            return nil;
        }
        along := x*dx + y*dy;
        if (along < lo) {
            lo = along;
            left = Point(path[i].x, path[i].y);
        }
        if (along > hi) {
            hi = along;
            right = Point(path[i].x, path[i].y);
        }
    }
    return brushround(left, right, stroke.width);
}


# EN: Build capsule sides and four tangent caps between copied endpoints.
# No control polygon point may leave the document page. Nil requests clipping.
# RU: Построить стороны капсулы и четыре касательные дуги по концам штриха.
# Все управляющие точки лежат в пределах листа. Nil требует обрезки.
brushround(left, right: Point, diameter: int): array of Node
{
    radius := real diameter / 2.0;
    bound := (diameter + 1) / 2;
    if (!pointok(left.x-bound, left.y-bound) || !pointok(right.x-bound, right.y-bound) ||
        !pointok(left.x+bound, left.y+bound) || !pointok(right.x+bound, right.y+bound)) {
        return nil;
    }
    dx := right.x-left.x;
    dy := right.y-left.y;
    tx := radius;
    ty := 0.0;
    moved := dx != 0 || dy != 0;
    if (moved) {
        math := load Math Math->PATH;
        if (math == nil) {
            return nil;
        }
        scale := radius / math->sqrt(real (dx*dx+dy*dy));
        tx = real dx * scale;
        ty = real dy * scale;
    }
    nx := ty;
    ny := -tx;
    # Limbo's real-to-int conversion rounds to nearest. A half-subpixel
    # nib remains an integer SVG geometry, just like other tools.
    nodes := array[6+2*moved] of Node;
    nodes[0] = Node(Move, int (real left.x+nx), int (real left.y+ny), 0, 0, 0, 0);
    at := 1;
    if (moved) {
        nodes[at++] = Node(Segment, int (real right.x+nx), int (real right.y+ny), 0, 0, 0, 0);
    }
    nodes[at++] = brusharc(right, nx, ny, tx, ty);
    nodes[at++] = brusharc(right, tx, ty, -nx, -ny);
    if (moved) {
        nodes[at++] = Node(Segment, int (real left.x-nx), int (real left.y-ny), 0, 0, 0, 0);
    }
    nodes[at++] = brusharc(left, -nx, -ny, -tx, -ty);
    nodes[at++] = brusharc(left, -tx, -ty, nx, ny);
    nodes[at] = Node(Close, 0, 0, 0, 0, 0, 0);
    return nodes;
}
