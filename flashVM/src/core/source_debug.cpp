/* FlashVM: independent bounded SWD reader, DebugID matching and source inspection. */
#include "flashvm/source_debug.h"
#include "../swf/decode.hpp"
#include "../swf/record_constants.h"
#include "crash_internal.hpp"
#include "source_debug_constants.h"
#include <cstdlib>
#include <cstring>
#include <new>
namespace
{
/* Parsed source bytes are copied, never interpreted as paths or executed. */
struct Module
{
    unsigned long lines;           /* Number of source lines, validated once during import. */
    unsigned long id;              /* SWD module identity. */
    fvm_detail::string name, text; /* Owned author-supplied filename and source contents. */
    Module() : lines(1), id(0) {}  /* Initialize a module awaiting a script record. */
};
/* POD line records can be sorted without moving owned strings. */
struct Line
{
    unsigned long module, line, offset; /* Module ID, one-based line and absolute SWF offset. */
};
/* A register declaration applies from its absolute code position. */
struct Register
{
    unsigned long offset;              /* Absolute uncompressed SWF offset. */
    unsigned int slot;                 /* AVM1 register identity. */
    fvm_detail::string name;           /* Owned source local name. */
    Register() : offset(0), slot(0) {} /* Initialize an unused register descriptor. */
};
/* Sticky bounds are represented by a caught status, never by reading beyond input. */
struct Reader
{
    const unsigned char *p; /* Borrowed symbol input. */
    size_t size, at;        /* Input size and next unread byte. */
    Reader(const void *data, size_t n) : p((const unsigned char *)data), size(n), at(0) {}
    /* Validate a span before consuming any byte. */
    void require(size_t n)
    {
        if (n > size - at)
            throw FVM_INVALID_ARGUMENT;
    }
    /* Read little-endian SWD integers without unaligned pointer dereferences. */
    unsigned long number(unsigned int n)
    {
        require(n);
        unsigned long v = 0;
        for (unsigned int i = 0; i < n; ++i)
            v |= (unsigned long)p[at++] << (FVM_SWD_BYTE_BITS * i);
        return v;
    }
    /* Read terminated author bytes with an input-bounded terminator search. */
    fvm_detail::string string()
    {
        require(1);
        const void *end = std::memchr(p + at, 0, size - at);
        if (!end)
            throw FVM_INVALID_ARGUMENT;
        size_t n = (const unsigned char *)end - (p + at);
        fvm_detail::string v((const char *)p + at, n);
        at += n + 1;
        return v;
    }
};
/* Sort equal offsets deterministically; conflicting source records are rejected later. */
int compare_lines(const void *left, const void *right)
{
    const Line &a = *(const Line *)left, &b = *(const Line *)right;
    return a.offset < b.offset ? -1 : a.offset > b.offset ? 1 : 0;
}
} // namespace
/* One owned symbol bundle; replacement is atomic and never retains caller buffers. */
struct FVM_DebugSymbols
{
    fvm_detail::vector<unsigned char> swf; /* Canonical decompressed document bytes. */
    fvm_detail::map<unsigned long, size_t>
        module_index;                       /* Module IDs resolve without quadratic scans. */
    fvm_detail::vector<Module> modules;     /* Embedded source files. */
    fvm_detail::vector<Line> lines;         /* Offset-sorted line records. */
    fvm_detail::vector<Register> registers; /* Materialized register names and code positions. */
};
/* Release copied symbols without touching managed objects. */
void fvm_debug_symbols_destroy(FVM_DebugSymbols *p) { delete p; }
namespace
{
/* Find exactly one root DebugID in the canonical SWF tag stream. */
const unsigned char *debug_id(const fvm_detail::vector<unsigned char> &swf)
{
    if (swf.size() <= FVM_SWF_PREFIX_BYTES)
        throw FVM_INVALID_ARGUMENT;
    unsigned int bits = swf[FVM_SWF_PREFIX_BYTES] >> FVM_SWF_RECT_WIDTH_SHIFT;
    size_t at =
        FVM_SWF_PREFIX_BYTES +
        (FVM_SWF_RECT_WIDTH_BITS + FVM_SWF_RECT_COORDINATES * bits + FVM_SWF_BYTE_BITS - 1) /
            FVM_SWF_BYTE_BITS +
        FVM_SWF_FRAME_FIELDS_BYTES;
    const unsigned char *id = 0;
    while (at < swf.size())
    {
        Reader in(swf.begin() + at, swf.size() - at);
        unsigned long tag = in.number(FVM_SWF_TAG_HEADER_BYTES);
        size_t length = (size_t)(tag & FVM_SWF_TAG_LENGTH_MASK);
        unsigned int code = (unsigned int)(tag >> FVM_SWF_TAG_CODE_SHIFT);
        if (length == FVM_SWF_TAG_LENGTH_MASK)
            length = (size_t)in.number(FVM_SWF_LONG_LENGTH_BYTES);
        in.require(length);
        at += in.at;
        if (code == FVM_SWF_TAG_DEBUG_ID)
        {
            if (id || length != FVM_SWF_DEBUG_ID_BYTES)
                throw FVM_INVALID_ARGUMENT;
            id = swf.begin() + at;
        }
        at += length;
        if (code == FVM_SWF_TAG_END)
            break;
    }
    if (!id)
        throw FVM_NOT_FOUND;
    return id;
}
/* Resolve a declared module without silently accepting dangling line references. */
const Module *module(const FVM_DebugSymbols &s, unsigned long id)
{
    fvm_detail::map<unsigned long, size_t>::const_iterator found = s.module_index.find(id);
    return found == s.module_index.end() ? 0 : &s.modules[found->second];
}
/* Return an exact one-based source line, recognizing CRLF, CR and LF. */
bool line_text(const Module &m, unsigned long line, const char *&out, size_t &size)
{
    if (!line)
        return false;
    size_t at = 0;
    unsigned long current = 1;
    while (current < line && at < m.text.size())
    {
        unsigned char c = (unsigned char)m.text[at++];
        if (c == '\r')
        {
            if (at < m.text.size() && m.text[at] == '\n')
                ++at;
            ++current;
        }
        else if (c == '\n')
            ++current;
    }
    if (current != line)
        return false;
    size_t end = at;
    while (end < m.text.size() && m.text[end] != '\r' && m.text[end] != '\n')
        ++end;
    out = m.text.data() + at;
    size = end - at;
    return true;
}
/* Parse independently of record order, then validate all associations before publishing. */
void parse(FVM_DebugSymbols &s, const void *raw, size_t size, const unsigned char *expected)
{
    Reader in(raw, size);
    in.require(FVM_SWD_HEADER_BYTES);
    if (in.p[0] != 'F' || in.p[1] != 'W' || in.p[2] != 'D')
        throw FVM_INVALID_ARGUMENT;
    if (in.p[FVM_SWD_VERSION_INDEX] < FVM_SWD_MIN_VERSION || in.p[FVM_SWD_VERSION_INDEX] > FVM_SWD_MAX_VERSION)
        throw FVM_UNSUPPORTED;
    in.at = FVM_SWD_HEADER_BYTES;
    bool has_id = false, ended = false;
    unsigned long records = 0;
    while (in.at < in.size)
    {
        if (++records > FVM_SWD_MAX_RECORDS)
            throw FVM_UNSUPPORTED;
        unsigned long tag = in.number(FVM_SWD_U32_BYTES);
        if (tag == FVM_SWD_END)
        {
            ended = true;
            break;
        }
        if (tag == FVM_SWD_ID)
        {
            in.require(FVM_SWF_DEBUG_ID_BYTES);
            if (has_id || std::memcmp(in.p + in.at, expected, FVM_SWF_DEBUG_ID_BYTES))
                throw FVM_INVALID_ARGUMENT;
            has_id = true;
            in.at += FVM_SWF_DEBUG_ID_BYTES;
        }
        else if (tag == FVM_SWD_SCRIPT)
        {
            Module m;
            m.id = in.number(FVM_SWD_U32_BYTES);
            in.number(FVM_SWD_U32_BYTES);
            m.name = in.string();
            m.text = in.string();
            if (module(s, m.id))
                throw FVM_INVALID_ARGUMENT;
            for (size_t i = 0; i < m.text.size(); ++i)
            {
                if (m.text[i] == '\r')
                {
                    if (i + 1 < m.text.size() && m.text[i + 1] == '\n')
                        ++i;
                    ++m.lines;
                }
                else if (m.text[i] == '\n')
                    ++m.lines;
            }
            s.module_index[m.id] = s.modules.size();
            s.modules.push_back(m);
        }
        else if (tag == FVM_SWD_OFFSET)
        {
            Line l;
            l.module = in.number(FVM_SWD_U32_BYTES);
            l.line = in.number(FVM_SWD_U32_BYTES);
            l.offset = in.number(FVM_SWD_U32_BYTES);
            if (l.offset >= s.swf.size() || !l.line)
                throw FVM_INVALID_ARGUMENT;
            s.lines.push_back(l);
        }
        else if (tag == FVM_SWD_BREAKPOINT)
        {
            if (in.number(FVM_SWD_U32_BYTES) >= s.swf.size())
                throw FVM_INVALID_ARGUMENT;
        }
        else if (tag == FVM_SWD_REGISTERS)
        {
            unsigned long offset = in.number(FVM_SWD_U32_BYTES);
            unsigned int count = (unsigned int)in.number(FVM_SWD_U8_BYTES);
            if (offset >= s.swf.size() || count > FVM_SWD_MAX_RECORDS - s.registers.size())
                throw FVM_INVALID_ARGUMENT;
            for (unsigned int i = 0; i < count; ++i)
            {
                Register r;
                r.offset = offset;
                r.slot = (unsigned int)in.number(FVM_SWD_U8_BYTES);
                r.name = in.string();
                s.registers.push_back(r);
            }
        }
        else
            throw FVM_UNSUPPORTED;
    }
    if (!ended || !has_id || in.at != in.size)
        throw FVM_INVALID_ARGUMENT;
    if (s.lines.size())
        std::qsort(s.lines.begin(), s.lines.size(), sizeof(Line), compare_lines);
    for (size_t i = 0; i < s.lines.size(); ++i)
    {
        const Line &l = s.lines[i];
        const Module *m = module(s, l.module);
        if (!m || l.line > m->lines)
            throw FVM_INVALID_ARGUMENT;
        if (i && l.offset == s.lines[i - 1].offset &&
            (l.module != s.lines[i - 1].module || l.line != s.lines[i - 1].line))
            throw FVM_INVALID_ARGUMENT;
    }
}
/* Find the nearest line at/before an absolute action offset within this body span. */
const Line *nearest(const FVM_DebugSymbols &s, size_t start, unsigned long offset)
{
    size_t low = 0, high = s.lines.size(), absolute = start + (size_t)offset;
    while (low < high)
    {
        size_t mid = low + (high - low) / 2;
        if (s.lines[mid].offset <= absolute)
            low = mid + 1;
        else
            high = mid;
    }
    if (!low || s.lines[low - 1].offset < start)
        return 0;
    return &s.lines[low - 1];
}
/* A cold bounded content lookup supports owned/copy-transported AVM1 bodies without name
 * collisions. */
FVM_Result find(FVM_DebugSymbols &s, const void *raw, size_t size, unsigned long offset,
                const Line *&line, size_t &body_start, bool unique = false)
{
    if (!raw || !size || (size_t)offset >= size)
        return FVM_INVALID_ARGUMENT;
    if (size > s.swf.size())
        return FVM_NOT_FOUND;
    const unsigned char *bytes = (const unsigned char *)raw;
    size_t at = 0, work = 0, matches = 0;
    line = 0;
    while (at <= s.swf.size() - size)
    {
        const void *hit = std::memchr(s.swf.begin() + at, bytes[0], s.swf.size() - size - at + 1);
        if (!hit)
            break;
        at = (const unsigned char *)hit - s.swf.begin();
        if (++matches > FVM_SWD_MAX_MATCHES || size > FVM_SWD_LOOKUP_WORK - work)
            return FVM_UNSUPPORTED;
        work += size;
        if (!std::memcmp(s.swf.begin() + at, raw, size))
        {
            const Line *candidate = nearest(s, at, offset);
            if (candidate)
            {
                if (line && unique && body_start != at)
                    return FVM_NOT_FOUND;
                if (line && (line->module != candidate->module || line->line != candidate->line))
                    return FVM_NOT_FOUND;
                line = candidate;
                body_start = at;
            }
        }
        ++at;
    }
    return line ? FVM_OK : FVM_NOT_FOUND;
}
} // namespace
/* Attach fully parsed, matching source metadata atomically; failed imports retain old symbols. */
extern "C" FVM_Result fvm_debug_load_swd(FVM_Context *c, const void *swf, size_t swf_size,
                                         const void *swd, size_t swd_size)
{
    if (!c || c->destroying || c->gc_releasing || c->avm1_execution_depth || c->crash.active ||
        c->crash.busy || !swf || !swd || !swd_size || swd_size > FVM_SWD_MAX_BYTES)
        return FVM_INVALID_ARGUMENT;
    FVM_DebugSymbols *s = 0;
    try
    {
        s = new FVM_DebugSymbols;
        FVM_SWFInfo info;
        info.struct_size = sizeof(info);
        FVM_Result result = fvm_swf_decode(swf, swf_size, FVM_SWF_DEFAULT_LIMIT, s->swf, &info);
        if (result != FVM_OK)
            throw result;
        parse(*s, swd, swd_size, debug_id(s->swf));
        delete c->crash.symbols;
        c->crash.symbols = s;
        return FVM_OK;
    }
    catch (FVM_Result result)
    {
        delete s;
        return result;
    }
    catch (...)
    {
        delete s;
        return FVM_OUT_OF_MEMORY;
    }
}
/* Discard symbols only outside live execution/debugger views. */
extern "C" FVM_Result fvm_debug_clear_symbols(FVM_Context *c)
{
    if (!c || c->destroying || c->avm1_execution_depth || c->crash.active || c->crash.busy)
        return FVM_INVALID_ARGUMENT;
    delete c->crash.symbols;
    c->crash.symbols = 0;
    return FVM_OK;
}
/* Expose embedded module count without loading external source paths. */
extern "C" FVM_Result fvm_debug_source_count(FVM_Context *c, size_t *count)
{
    if (!c || !count)
        return FVM_INVALID_ARGUMENT;
    *count = c->crash.symbols ? c->crash.symbols->modules.size() : 0;
    return FVM_OK;
}
/* Return one immutable SWD source file. */
extern "C" FVM_Result fvm_debug_source_at(FVM_Context *c, size_t index, unsigned long *id,
                                          const char **name, size_t *name_size, const char **text,
                                          size_t *text_size)
{
    if (!c || !id || !name || !name_size || !text || !text_size)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.symbols || index >= c->crash.symbols->modules.size())
        return FVM_NOT_FOUND;
    const Module &m = c->crash.symbols->modules[index];
    *id = m.id;
    *name = m.name.data();
    *name_size = m.name.size();
    *text = m.text.data();
    *text_size = m.text.size();
    return FVM_OK;
}
/* Resolve a copied body only when its DebugID-associated mapping is unambiguous. */
extern "C" FVM_Result fvm_debug_source_position(FVM_Context *c, const void *body, size_t size,
                                                unsigned long offset, FVM_DebugSourcePosition *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.symbols)
        return FVM_NOT_FOUND;
    FVM_DebugSymbols &s = *c->crash.symbols;
    const Line *line;
    size_t start = 0;
    FVM_Result result = find(s, body, size, offset, line, start);
    if (result != FVM_OK)
        return result;
    const Module *m = module(s, line->module);
    FVM_DebugSourcePosition p;
    p.module_id = m->id;
    p.source = m->name.data();
    p.source_size = m->name.size();
    p.line = line->line;
    if (!line_text(*m, line->line, p.line_text, p.line_size))
        return FVM_NOT_FOUND;
    *out = p;
    return FVM_OK;
}
/* Current source lookup is a cold debugger operation, not an instruction-time scan. */
extern "C" FVM_Result fvm_debug_current_source(FVM_Context *c, FVM_DebugSourcePosition *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.active || c->crash.active->external)
        return FVM_NOT_FOUND;
    return fvm_debug_source_position(c, c->crash.active->source, c->crash.active->source_size,
                                     c->debug_offset, out);
}
/* Recover a declared register name only within the matched body's source region. */
extern "C" FVM_Result fvm_debug_register_name(FVM_Context *c, const void *body, size_t size,
                                              unsigned long offset, unsigned int reg,
                                              const char **name, size_t *name_size)
{
    if (!c || !name || !name_size)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.symbols)
        return FVM_NOT_FOUND;
    FVM_DebugSymbols &s = *c->crash.symbols;
    const Line *line;
    size_t start = 0;
    FVM_Result result = find(s, body, size, offset, line, start, true);
    if (result != FVM_OK)
        return result;
    const Register *best = 0;
    bool ambiguous = false;
    for (size_t i = 0; i < s.registers.size(); ++i)
    {
        const Register &r = s.registers[i];
        if (r.slot != reg || r.offset < start || r.offset > start + offset)
            continue;
        if (!best || r.offset > best->offset)
        {
            best = &r;
            ambiguous = false;
        }
        else if (r.offset == best->offset && r.name != best->name)
            ambiguous = true;
    }
    if (!best || ambiguous)
        return FVM_NOT_FOUND;
    *name = best->name.data();
    *name_size = best->name.size();
    return FVM_OK;
}
