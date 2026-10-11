#include "flashvm/path.h"
#include "../details/vector.hpp"
#include <cmath>
#include <cstring>
#include <climits>
#include <new>
#ifdef FVM_HAS_AGG
#include "agg_path_storage.h"
#include "agg_conv_curve.h"
#include "agg_conv_stroke.h"
#include "agg_pixfmt_rgba.h"
#include "agg_renderer_scanline.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_u.h"
#elif defined(FVM_HAS_CAIRO)
#include "cairo.h"
#endif
struct FVM_Path
{
    fvm_detail::vector<FVM_PathCommand> commands;
    bool open;
    FVM_Path() : open(false)
    {
    }
};
static unsigned int points(FVM_PathVerb v)
{
    return v == FVM_PATH_CLOSE ? 0 : v == FVM_PATH_CUBIC ? 3 : v == FVM_PATH_QUADRATIC ? 2 : 1;
}
static bool coordinate(double x)
{
    return x >= -1000000 && x <= 1000000;
}
extern "C" FVM_Result fvm_path_create(FVM_Path **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = new (std::nothrow) FVM_Path;
    return *out ? FVM_OK : FVM_OUT_OF_MEMORY;
}
extern "C" void fvm_path_destroy(FVM_Path *p)
{
    delete p;
}
extern "C" size_t fvm_path_count(const FVM_Path *p)
{
    return p ? p->commands.size() : 0;
}
extern "C" FVM_Result fvm_path_get(const FVM_Path *p, size_t i, FVM_PathCommand *out)
{
    if (!p || !out || i >= p->commands.size())
        return FVM_INVALID_ARGUMENT;
    *out = p->commands[i];
    return FVM_OK;
}
extern "C" FVM_Result fvm_path_add(FVM_Path *p, const FVM_PathCommand *c)
{
    if (!p || !c || c->verb < FVM_PATH_MOVE || c->verb > FVM_PATH_CLOSE ||
        (c->verb != FVM_PATH_MOVE && !p->open) || p->commands.size() >= 1000000)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < points(c->verb); ++i)
        if (!coordinate(c->x[i]) || !coordinate(c->y[i]))
            return FVM_INVALID_ARGUMENT;
    FVM_PathCommand copy;
    std::memset(&copy, 0, sizeof(copy));
    copy.verb = c->verb;
    for (unsigned int i = 0; i < points(c->verb); ++i)
    {
        copy.x[i] = c->x[i];
        copy.y[i] = c->y[i];
    }
    try
    {
        p->commands.push_back(copy);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    p->open = c->verb != FVM_PATH_CLOSE;
    return FVM_OK;
}
extern "C" FVM_Result fvm_path_set_point(FVM_Path *p, size_t i, unsigned int n, double x, double y)
{
    if (!p || i >= p->commands.size() || n >= points(p->commands[i].verb) || !coordinate(x) ||
        !coordinate(y))
        return FVM_INVALID_ARGUMENT;
    p->commands[i].x[n] = x;
    p->commands[i].y[n] = y;
    return FVM_OK;
}
static bool transform_command(FVM_PathCommand &c, const FVM_Affine *m)
{
    if (!m)
        return true;
    for (unsigned int n = 0; n < points(c.verb); ++n)
    {
        double x = m->xx * c.x[n] + m->xy * c.y[n] + m->x0;
        double y = m->yx * c.x[n] + m->yy * c.y[n] + m->y0;
        if (!coordinate(x) || !coordinate(y))
            return false;
        c.x[n] = x;
        c.y[n] = y;
    }
    return true;
}
static bool valid_matrix(const FVM_Affine *m)
{
    return m && coordinate(m->xx) && coordinate(m->xy) && coordinate(m->yx) && coordinate(m->yy) &&
           coordinate(m->x0) && coordinate(m->y0);
}
extern "C" FVM_Result fvm_path_transform(FVM_Path *p, const FVM_Affine *m)
{
    if (!p || !valid_matrix(m))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FVM_PathCommand> copy = p->commands;
        for (size_t i = 0; i < copy.size(); ++i)
            if (!transform_command(copy[i], m))
                return FVM_INVALID_ARGUMENT;
        p->commands.swap(copy);
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_path_append(FVM_Path *p, const FVM_Path *source, const FVM_Affine *m)
{
    if (!p || !source || (m && !valid_matrix(m)) ||
        source->commands.size() > 1000000 - p->commands.size())
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FVM_PathCommand> copy = p->commands;
        for (size_t i = 0; i < source->commands.size(); ++i)
        {
            FVM_PathCommand c = source->commands[i];
            if (!transform_command(c, m))
                return FVM_INVALID_ARGUMENT;
            copy.push_back(c);
        }
        bool open = source->commands.empty() ? p->open : source->open;
        p->commands.swap(copy);
        p->open = open;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
static void extent(double v, double &low, double &high)
{
    if (v < low)
        low = v;
    if (v > high)
        high = v;
}
static double bezier(double p0, double p1, double p2, double p3, double t, bool cubic)
{
    double s = 1 - t;
    return cubic ? s * s * s * p0 + 3 * s * s * t * p1 + 3 * s * t * t * p2 + t * t * t * p3
                 : s * s * p0 + 2 * s * t * p1 + t * t * p2;
}
static void curve_bounds(double p0, double p1, double p2, double p3, bool cubic, double &low,
                         double &high)
{
    extent(cubic ? p3 : p2, low, high);
    double a = cubic ? -p0 + 3 * p1 - 3 * p2 + p3 : 0;
    double b = cubic ? 2 * (p0 - 2 * p1 + p2) : p0 - 2 * p1 + p2;
    double c = p1 - p0;
    double roots[2];
    unsigned int n = 0;
    if (a == 0)
    {
        if (b != 0)
            roots[n++] = -c / b;
    }
    else
    {
        double d = b * b - 4 * a * c;
        if (d >= 0)
        {
            double q = -0.5 * (b + (b < 0 ? -std::sqrt(d) : std::sqrt(d)));
            if (q == 0)
                roots[n++] = -b / (2 * a);
            else
            {
                roots[n++] = q / a;
                roots[n++] = c / q;
            }
        }
    }
    for (unsigned int i = 0; i < n; ++i)
        if (roots[i] > 0 && roots[i] < 1)
            extent(bezier(p0, p1, p2, p3, roots[i], cubic), low, high);
}
extern "C" FVM_Result fvm_path_bounds(const FVM_Path *p, FVM_PathBounds *out)
{
    if (!p || !out)
        return FVM_INVALID_ARGUMENT;
    FVM_PathBounds box = {0, 0, 0, 0};
    double x = 0, y = 0, sx = 0, sy = 0;
    bool first = true;
    for (size_t i = 0; i < p->commands.size(); ++i)
    {
        const FVM_PathCommand &c = p->commands[i];
        if (c.verb == FVM_PATH_CLOSE)
        {
            x = sx;
            y = sy;
            continue;
        }
        unsigned int n = points(c.verb);
        if (first)
        {
            box.x_min = box.x_max = c.x[0];
            box.y_min = box.y_max = c.y[0];
            first = false;
        }
        if (c.verb == FVM_PATH_MOVE || c.verb == FVM_PATH_LINE)
        {
            extent(c.x[0], box.x_min, box.x_max);
            extent(c.y[0], box.y_min, box.y_max);
        }
        else
        {
            curve_bounds(x, c.x[0], c.x[1], c.x[2], n == 3, box.x_min, box.x_max);
            curve_bounds(y, c.y[0], c.y[1], c.y[2], n == 3, box.y_min, box.y_max);
        }
        x = c.x[n - 1];
        y = c.y[n - 1];
        if (c.verb == FVM_PATH_MOVE)
        {
            sx = x;
            sy = y;
        }
    }
    *out = box;
    return FVM_OK;
}
static bool valid_surface(const FVM_RasterSurface *s)
{
    return s && s->pixels && s->width && s->height && s->width <= 4096 && s->height <= 4096 &&
           s->stride >= (size_t)s->width * 4 && s->stride <= (size_t)INT_MAX / s->height;
}
#ifdef FVM_HAS_CAIRO
static unsigned int packed_pixel(const unsigned char *p)
{
    unsigned int a = p[3];
    return (a << 24) | (((p[0] * a + 127) / 255) << 16) | (((p[1] * a + 127) / 255) << 8) |
           ((p[2] * a + 127) / 255);
}
#endif
extern "C" FVM_Result fvm_path_draw(const FVM_Path *p, FVM_RasterSurface *s, FVM_RasterColor color,
                                    double width, int even_odd)
{
    if (!p || !valid_surface(s) || !(width >= 0 && width <= 4096) ||
        (even_odd != 0 && even_odd != 1))
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < p->commands.size(); ++i)
        for (unsigned int n = 0; n < points(p->commands[i].verb); ++n)
            if (!(p->commands[i].x[n] >= -4096 && p->commands[i].x[n] <= 4096) ||
                !(p->commands[i].y[n] >= -4096 && p->commands[i].y[n] <= 4096))
                return FVM_INVALID_ARGUMENT;
#ifdef FVM_HAS_AGG
    try
    {
        agg::path_storage path;
        for (size_t i = 0; i < p->commands.size(); ++i)
        {
            const FVM_PathCommand &c = p->commands[i];
            switch (c.verb)
            {
            case FVM_PATH_MOVE:
                path.move_to(c.x[0], c.y[0]);
                break;
            case FVM_PATH_LINE:
                path.line_to(c.x[0], c.y[0]);
                break;
            case FVM_PATH_QUADRATIC:
                path.curve3(c.x[0], c.y[0], c.x[1], c.y[1]);
                break;
            case FVM_PATH_CUBIC:
                path.curve4(c.x[0], c.y[0], c.x[1], c.y[1], c.x[2], c.y[2]);
                break;
            case FVM_PATH_CLOSE:
                path.close_polygon();
                break;
            }
        }
        agg::conv_curve<agg::path_storage> curves(path);
        agg::rasterizer_scanline_aa<> ras;
        agg::scanline_u8 scanline;
        ras.clip_box(0, 0, s->width, s->height);
        ras.filling_rule(even_odd ? agg::fill_even_odd : agg::fill_non_zero);
        if (width > 0)
        {
            agg::conv_stroke<agg::conv_curve<agg::path_storage> > stroke(curves);
            stroke.width(width);
            stroke.line_join(agg::round_join);
            stroke.line_cap(agg::round_cap);
            ras.add_path(stroke);
        }
        else
            ras.add_path(curves);
        agg::rendering_buffer buffer(s->pixels, s->width, s->height, (int)s->stride);
        agg::pixfmt_rgba32_plain pixels(buffer);
        agg::renderer_base<agg::pixfmt_rgba32_plain> renderer(pixels);
        agg::render_scanlines_aa_solid(ras, scanline, renderer,
                                       agg::rgba8(color.r, color.g, color.b, color.a));
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
#elif defined(FVM_HAS_CAIRO)
    cairo_surface_t *image =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)s->width, (int)s->height);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(image);
        return FVM_OUT_OF_MEMORY;
    }
    unsigned char *data = cairo_image_surface_get_data(image);
    int stride = cairo_image_surface_get_stride(image);
    for (unsigned int y = 0; y < s->height; ++y)
        for (unsigned int x = 0; x < s->width; ++x)
        {
            unsigned int pixel = packed_pixel(s->pixels + y * s->stride + x * 4);
            std::memcpy(data + y * stride + x * 4, &pixel, 4);
        }
    cairo_surface_mark_dirty(image);
    cairo_t *cr = cairo_create(image);
    double x = 0, y = 0, sx = 0, sy = 0;
    for (size_t i = 0; i < p->commands.size(); ++i)
    {
        const FVM_PathCommand &c = p->commands[i];
        switch (c.verb)
        {
        case FVM_PATH_MOVE:
            cairo_move_to(cr, c.x[0], c.y[0]);
            sx = c.x[0];
            sy = c.y[0];
            break;
        case FVM_PATH_LINE:
            cairo_line_to(cr, c.x[0], c.y[0]);
            break;
        case FVM_PATH_QUADRATIC:
            cairo_curve_to(cr, x + 2 * (c.x[0] - x) / 3, y + 2 * (c.y[0] - y) / 3,
                           c.x[1] + 2 * (c.x[0] - c.x[1]) / 3, c.y[1] + 2 * (c.y[0] - c.y[1]) / 3,
                           c.x[1], c.y[1]);
            break;
        case FVM_PATH_CUBIC:
            cairo_curve_to(cr, c.x[0], c.y[0], c.x[1], c.y[1], c.x[2], c.y[2]);
            break;
        case FVM_PATH_CLOSE:
            cairo_close_path(cr);
            break;
        }
        if (c.verb == FVM_PATH_CLOSE)
        {
            x = sx;
            y = sy;
        }
        else
        {
            unsigned int n = points(c.verb);
            x = c.x[n - 1];
            y = c.y[n - 1];
        }
    }
    cairo_set_source_rgba(cr, color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
    cairo_set_fill_rule(cr, even_odd ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    if (width > 0)
    {
        cairo_set_line_width(cr, width);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke(cr);
    }
    else
        cairo_fill(cr);
    cairo_status_t status = cairo_status(cr);
    cairo_destroy(cr);
    if (status != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(image);
        return status == CAIRO_STATUS_NO_MEMORY ? FVM_OUT_OF_MEMORY : FVM_ERROR;
    }
    cairo_surface_flush(image);
    for (unsigned int y = 0; y < s->height; ++y)
        for (unsigned int x = 0; x < s->width; ++x)
        {
            unsigned char *rgba = s->pixels + y * s->stride + x * 4;
            unsigned int pixel;
            std::memcpy(&pixel, data + y * stride + x * 4, 4);
            if (pixel == packed_pixel(rgba))
                continue;
            unsigned int a = pixel >> 24;
            rgba[3] = (unsigned char)a;
            for (unsigned int c = 0; c < 3; ++c)
            {
                unsigned int component = (pixel >> (16 - c * 8)) & 255;
                unsigned int straight = a ? (component * 255 + a / 2) / a : 0;
                rgba[c] = (unsigned char)(straight > 255 ? 255 : straight);
            }
        }
    cairo_surface_destroy(image);
    return FVM_OK;
#else
    (void)color;
    return FVM_UNSUPPORTED;
#endif
}
extern "C" FVM_Result fvm_path_mask(const FVM_Path *p, FVM_RasterSurface *dst,
                                    const FVM_RasterSurface *src, int even_odd)
{
    if (!valid_surface(dst) || !valid_surface(src) || src->width != dst->width ||
        src->height != dst->height)
        return FVM_INVALID_ARGUMENT;
    size_t size = (size_t)dst->width * dst->height * 4;
    unsigned char *mask = new (std::nothrow) unsigned char[size];
    if (!mask)
        return FVM_OUT_OF_MEMORY;
    std::memset(mask, 0, size);
    FVM_RasterSurface s = {mask, dst->width, dst->height, (size_t)dst->width * 4};
    FVM_RasterColor white = {255, 255, 255, 255};
    FVM_Result result = fvm_path_draw(p, &s, white, 0, even_odd);
    if (result == FVM_OK)
        for (unsigned int y = 0; y < dst->height; ++y)
            for (unsigned int x = 0; x < dst->width; ++x)
            {
                unsigned char *d = dst->pixels + y * dst->stride + x * 4;
                const unsigned char *v = src->pixels + y * src->stride + x * 4;
                unsigned int a = (v[3] * mask[((size_t)y * dst->width + x) * 4 + 3] + 127) / 255;
                if (!a)
                    continue;
                unsigned int alpha = a * 255 + d[3] * (255 - a);
                for (unsigned int c = 0; c < 3; ++c)
                    d[c] = (unsigned char)((v[c] * a * 255 + d[c] * d[3] * (255 - a) + alpha / 2) /
                                           alpha);
                d[3] = (unsigned char)((alpha + 127) / 255);
            }
    delete[] mask;
    return result;
}
