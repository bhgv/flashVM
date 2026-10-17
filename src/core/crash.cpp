/* FlashVM: best-effort raw fault snapshots, ZIP32 storage and live debugger queries. */
#include "crash_constants.h"
#include "crash_internal.hpp"
#include "flashvm/jit.h"
#include <cstdio>
#include <cstring>
#include <new>
#include <zlib.h>

namespace
{
typedef fvm_detail::vector<unsigned char> Bytes;
/* Append bounded bytes; budget exhaustion has the same best-effort boundary as OOM. */
void append(Bytes &out, const void *raw, size_t n)
{
    if (n > FVM_CRASH_MAX_BYTES - out.size())
        throw std::bad_alloc();
    const unsigned char *p = (const unsigned char *)raw;
    for (size_t i = 0; i < n; ++i)
        out.push_back(p[i]);
}
/* Append a fixed ASCII JSON token. */
void text(Bytes &out, const char *s) { append(out, s, std::strlen(s)); }
/* Encode arbitrary counted identifier bytes as reversible JSON byte escapes. */
void quoted(Bytes &out, const char *s, size_t n)
{
    text(out, "\"");
    for (size_t i = 0; i < n; ++i)
    {
        unsigned char ch = (unsigned char)s[i];
        if (ch >= 32 && ch < 127 && ch != '"' && ch != '\\')
            append(out, &ch, 1);
        else
        {
            char escaped[7];
            std::sprintf(escaped, "\\u%04x", (unsigned int)ch);
            text(out, escaped);
        }
    }
    text(out, "\"");
}
/* Represent integer sizes without narrowing to a platform-specific printf size modifier. */
void integer(Bytes &out, unsigned long n)
{
    char b[32];
    std::sprintf(b, "%lu", n);
    text(out, b);
}
/* Pointer IDs are opaque snapshot-local object identities, never deserialization handles. */
void identity(Bytes &out, const void *p)
{
    char b[64];
    std::sprintf(b, "%p", p);
    quoted(out, b, std::strlen(b));
}
/* Format raw typed values without getters, coercion, graph traversal or script entry. */
void value(Bytes &out, const FVM_Value &v)
{
    text(out, "{\"type\":");
    integer(out, (unsigned long)v.type);
    text(out, ",\"value\":");
    if (v.type == FVM_VALUE_OBJECT)
        identity(out, v.object);
    else if (v.type == FVM_VALUE_STRING)
        quoted(out, v.string, fvm_value_string_size(&v));
    else if (v.type == FVM_VALUE_NUMBER)
    {
        char b[64];
        std::sprintf(b, "%.17g", v.number);
        quoted(out, b, std::strlen(b));
    }
    else if (v.type == FVM_VALUE_BOOLEAN)
        text(out, v.boolean ? "true" : "false");
    else
        text(out, "null");
    text(out, ",\"primitive_meta\":");
    identity(out, v.metatable);
    text(out, "}");
}
/* A section is copied independently so postmortem storage owns no live VM references. */
struct SavedFrame
{
    fvm_detail::string name;     /* Owned function label. */
    fvm_detail::string source;   /* Owned mapped source name. */
    unsigned long offset, line;  /* AVM1 offset and optional source line. */
    bool forced;                 /* Successful FORCE entry preservation. */
    bool jit;                    /* Actual engine at the failing activation. */
    const void *machine_address; /* Original RX address, retained only as an opaque identity. */
    unsigned int argc;           /* Supplied argument count. */
    Bytes bytecode, machine;     /* Owned source and optional generated instruction bytes. */
    /* Initialize copied frame metadata. */
    SavedFrame() : offset(0), line(0), forced(false), jit(false), machine_address(0), argc(0) {}
};
/* Encode a fixed-width little-endian ZIP field. */
void field(Bytes &out, unsigned long n, unsigned int width)
{
    for (unsigned int i = 0; i < width; ++i)
    {
        unsigned char b = (unsigned char)(n & FVM_CRASH_BYTE_MASK);
        append(out, &b, 1);
        n >>= FVM_CRASH_BYTE_BITS;
    }
}
/* Stored ZIP entries use CRC32; no compressor or third-party dynamic dependency is needed. */
struct Archive
{
    Bytes files, directory; /* Local records/payload and central directory. */
    unsigned int count;     /* Number of complete entries. */
    Archive() : count(0) {} /* Start an empty ZIP32 container. */
    /* Add one ASCII-named entry with complete size and CRC in both directory records. */
    void add(const char *name, const void *p, size_t n)
    {
        size_t offset = files.size(), length = std::strlen(name);
        unsigned long crc = (unsigned long)crc32(0, (const Bytef *)p, (uInt)n);
        field(files, FVM_CRASH_ZIP_LOCAL, 4);
        field(files, FVM_CRASH_ZIP_VERSION, 2);
        field(files, 0, 2);
        field(files, 0, 2);
        field(files, 0, 2);
        field(files, 0, 2);
        field(files, crc, 4);
        field(files, (unsigned long)n, 4);
        field(files, (unsigned long)n, 4);
        field(files, (unsigned long)length, 2);
        field(files, 0, 2);
        append(files, name, length);
        append(files, p, n);
        field(directory, FVM_CRASH_ZIP_CENTRAL, 4);
        field(directory, FVM_CRASH_ZIP_VERSION, 2);
        field(directory, FVM_CRASH_ZIP_VERSION, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, crc, 4);
        field(directory, (unsigned long)n, 4);
        field(directory, (unsigned long)n, 4);
        field(directory, (unsigned long)length, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, 0, 2);
        field(directory, 0, 4);
        field(directory, (unsigned long)offset, 4);
        append(directory, name, length);
        ++count;
    }
    /* Finish an archive only after all local entries have been built successfully. */
    void finish(Bytes &out)
    {
        unsigned long start = (unsigned long)files.size(), size = (unsigned long)directory.size();
        append(files, directory.begin(), directory.size());
        field(files, FVM_CRASH_ZIP_END, 4);
        field(files, 0, 2);
        field(files, 0, 2);
        field(files, count, 2);
        field(files, count, 2);
        field(files, size, 4);
        field(files, start, 4);
        field(files, 0, 2);
        out.swap(files);
    }
};
/* Resolve host mapping from immutable function name and AVM1 offset, including suspended callers.
 */
void location(FVM_Context *c, const FVM_CrashActivation &a, SavedFrame &out)
{
    out.name = a.name ? *a.name : fvm_detail::string("<root>");
    unsigned long nearest = 0;
    bool found = false;
    for (size_t i = 0; i < c->debug_locations.size(); ++i)
    {
        const FVM_SourceLocation &m = c->debug_locations[i];
        bool match = a.name ? m.function == *a.name : m.function.empty();
        if (match && m.offset <= out.offset && (!found || m.offset >= nearest))
        {
            found = true;
            nearest = m.offset;
            out.source = m.source;
            out.line = m.line;
        }
    }
}
/* Snapshot available raw local bindings; missing optimized locals remain explicitly unavailable. */
void locals(Bytes &out, FVM_Object *activation)
{
    if (!activation)
    {
        text(out, "null");
        return;
    }
    text(out, "[");
    bool comma = false;
    for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i =
             activation->properties.begin();
         i != activation->properties.end(); ++i)
    {
        if (comma)
            text(out, ",");
        comma = true;
        text(out, "{\"name\":");
        quoted(out, i->first.data(), i->first.size());
        text(out, ",\"value\":");
        value(out, i->second);
        text(out, "}");
    }
    text(out, "]");
}
/* Capture raw object/class fields and descriptor identities without calling user code. */
void heap(FVM_Context *c, Bytes &out)
{
    text(out, "{\"global\":");
    identity(out, c->global);
    text(out, ",\"objects\":[");
    for (size_t n = 0; n < c->objects.size(); ++n)
    {
        FVM_Object *o = c->objects[n];
        if (n)
            text(out, ",");
        text(out, "{\"id\":");
        identity(out, o);
        text(out, ",\"prototype\":");
        identity(out, o->prototype);
        text(out, ",\"metatable\":");
        identity(out, o->metatable);
        text(out, ",\"resource_kind\":");
        integer(out, (unsigned long)o->resource_kind);
        text(out, ",\"resource_result\":");
        if (o->resource_result_valid)
            value(out, o->resource_result);
        else
            text(out, "null");
        text(out, ",\"properties\":");
        locals(out, o);
        text(out, ",\"indexed\":[");
        bool comma = false;
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = o->indexed.begin();
             i != o->indexed.end(); ++i)
        {
            if (comma)
                text(out, ",");
            comma = true;
            text(out, "{\"index\":");
            integer(out, i->first);
            text(out, ",\"value\":");
            value(out, i->second);
            text(out, "}");
        }
        text(out, "],\"attributes\":[");
        comma = false;
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = o->attributes.begin();
             i != o->attributes.end(); ++i)
        {
            if (comma)
                text(out, ",");
            comma = true;
            text(out, "{\"name\":");
            quoted(out, i->first.data(), i->first.size());
            text(out, ",\"flags\":");
            integer(out, i->second);
            text(out, "}");
        }
        text(out, "],\"native_methods\":[");
        comma = false;
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i = o->methods.begin();
             i != o->methods.end(); ++i)
        {
            if (comma)
                text(out, ",");
            comma = true;
            quoted(out, i->first.data(), i->first.size());
        }
        text(out, "],\"accessors\":[");
        comma = false;
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                 o->accessors.begin();
             i != o->accessors.end(); ++i)
        {
            if (comma)
                text(out, ",");
            comma = true;
            quoted(out, i->first.data(), i->first.size());
        }
        text(out, "]}");
    }
    text(out, "]}");
}
} // namespace
/* Copied snapshots retain no managed pointers; only active debugger queries borrow live values. */
struct FVM_CrashReport
{
    fvm_detail::vector<SavedFrame> frames; /* Owned innermost-first body snapshots. */
    Bytes trace, zip;                      /* Persistent JSON/ZIP payloads. */
    FVM_Result status;                     /* Original execution failure. */
    FVM_CrashReport() : status(FVM_OK) {}  /* Initialize an empty postmortem report. */
};
/* Destroy copied byte storage without traversing managed state. */
FVM_CrashState::~FVM_CrashState() { delete report; }
/* Register a borrowed stack frame only for enabled diagnostics and meaningful activations. */
FVM_CrashScope::FVM_CrashScope(FVM_Context *c, const fvm_detail::string *name, const void *source,
                               size_t size, unsigned int argc, const FVM_Value *argv,
                               FVM_Object *activation, bool enabled)
    : context_(c), linked_(enabled && c->crash.mode != FVM_CRASH_NONE && !c->crash.busy &&
                           !c->execution_mode.replay_depth)
{
    rooted_ = false;
    if (!linked_)
        return;
    if (!c->crash.active)
        c->crash.preparation_status = FVM_OK;
    if (c->crash.active)
        c->crash.active->offset = c->debug_offset;
    frame_.previous = c->crash.active;
    frame_.name = name;
    frame_.source = (const unsigned char *)source;
    frame_.source_size = size;
    frame_.argc = argc;
    frame_.argv = argv;
    frame_.activation = activation;
    frame_.registers = 0;
    frame_.register_count = 0;
    frame_.machine = 0;
    frame_.machine_size = 0;
    frame_.jit = false;
    frame_.offset = 0;
    frame_.function = 0;
    frame_.forced = false;
    if (c->crash.detail == FVM_CRASH_DETAIL_FORCE)
    {
        try
        {
            arguments_.resize(argc);
            strings_.resize(argc);
            size_t copied = 0;
            for (unsigned int i = 0; i < argc; ++i)
            {
                arguments_[i] = argv[i];
                if (argv[i].type == FVM_VALUE_STRING)
                {
                    size_t n = fvm_value_string_size(argv + i);
                    if (n > FVM_CRASH_MAX_BYTES - copied)
                        throw std::bad_alloc();
                    copied += n;
                    strings_[i] = fvm_detail::string(argv[i].string, n);
                    arguments_[i].string = strings_[i].data();
                    arguments_[i].string_size = n;
                }
            }
            argument_roots_.previous = c->gc_frames;
            argument_roots_.stack = &arguments_;
            argument_roots_.locals = 0;
            argument_roots_.receiver = 0;
            c->gc_frames = &argument_roots_;
            rooted_ = true;
            frame_.argv = arguments_.begin();
            frame_.forced = true;
        }
        catch (...)
        {
            c->crash.preparation_status = FVM_OUT_OF_MEMORY;
        }
    }
    c->crash.active = &frame_;
    c->crash.propagating = false;
}
/* Pop borrowed links before their source/activation storage goes out of scope. */
FVM_CrashScope::~FVM_CrashScope()
{
    if (rooted_)
        context_->gc_frames = argument_roots_.previous;
    if (linked_)
        context_->crash.active = frame_.previous;
}
/* Build a best-effort report while all participating activation values remain rooted. */
void fvm_crash_failure(FVM_Context *c, FVM_Result status)
{
    FVM_CrashState &s = c->crash;
    if (status == FVM_OK || c->execution_mode.replay_depth || s.mode == FVM_CRASH_NONE || s.busy ||
        s.propagating || !s.active)
        return;
    s.propagating = true;
    s.busy = true;
    s.diagnostic_status = s.preparation_status;
    delete s.report;
    s.report = 0;
    FVM_CrashReport *r = 0;
    try
    {
        r = new FVM_CrashReport;
        r->status = status;
        text(r->trace, "{\"format\":1,\"first_chance\":true,\"status\":");
        char number[32];
        std::sprintf(number, "%d", (int)status);
        text(r->trace, number);
        text(r->trace, ",\"frames\":[");
        FVM_CrashActivation *a = s.active;
        size_t copied = 0;
        for (size_t n = 0; a && n < FVM_CRASH_MAX_FRAMES; ++n, a = a->previous)
        {
            SavedFrame saved;
            saved.offset = n ? a->offset : c->debug_offset;
            saved.forced = a->forced;
            saved.jit = a->jit;
            saved.machine_address = a->machine;
            saved.argc = a->argc;
            location(c, *a, saved);
            size_t native = s.mode == FVM_CRASH_ZIP_CODE ? a->machine_size : 0;
            if (a->source_size > FVM_CRASH_MAX_BYTES - copied ||
                native > FVM_CRASH_MAX_BYTES - copied - a->source_size)
                throw std::bad_alloc();
            copied += a->source_size + native;
            append(saved.bytecode, a->source, a->source_size);
            append(saved.machine, a->machine, native);
            r->frames.push_back(saved);
            if (n)
                text(r->trace, ",");
            text(r->trace, "{\"function\":");
            quoted(r->trace, saved.name.data(), saved.name.size());
            text(r->trace, ",\"engine\":");
            text(r->trace, a->jit ? "\"jit\"" : "\"interpreter\"");
            text(r->trace, ",\"jit_address\":");
            identity(r->trace, a->machine);
            text(r->trace, ",\"jit_size\":");
            integer(r->trace, (unsigned long)a->machine_size);
            text(r->trace, ",\"source\":");
            quoted(r->trace, saved.source.data(), saved.source.size());
            text(r->trace, ",\"line\":");
            integer(r->trace, saved.line);
            text(r->trace, ",\"offset\":");
            integer(r->trace, saved.offset);
            text(r->trace, ",\"forced\":");
            text(r->trace, a->forced ? "true" : "false");
            text(r->trace, ",\"arguments\":[");
            for (unsigned int j = 0; j < a->argc; ++j)
            {
                if (j)
                    text(r->trace, ",");
                value(r->trace, a->argv[j]);
            }
            text(r->trace, "],\"locals\":");
            if (s.mode == FVM_CRASH_BACKTRACE)
                text(r->trace, "null");
            else
                locals(r->trace, a->activation);
            if (a->forced)
            {
                text(r->trace, ",\"registers\":[");
                for (unsigned int j = 0; j < a->register_count; ++j)
                {
                    if (j)
                        text(r->trace, ",");
                    value(r->trace, a->registers[j]);
                }
                text(r->trace, "],\"register_locals\":[");
                bool comma = false;
                if (a->function && a->function->function2)
                    for (size_t j = 0; j < a->function->parameters.size(); ++j)
                    {
                        unsigned int reg = a->function->parameter_registers[j];
                        if (!reg || reg >= a->register_count)
                            continue;
                        if (comma)
                            text(r->trace, ",");
                        comma = true;
                        text(r->trace, "{\"name\":");
                        const fvm_detail::string &name = a->function->parameters[j];
                        quoted(r->trace, name.data(), name.size());
                        text(r->trace, ",\"register\":");
                        integer(r->trace, reg);
                        text(r->trace, ",\"value\":");
                        value(r->trace, a->registers[reg]);
                        text(r->trace, "}");
                    }
                text(r->trace, "]");
            }
            text(r->trace, "}");
        }
        text(r->trace, "],\"frames_truncated\":");
        text(r->trace, a ? "true}" : "false}");
        s.report = r;
        r = 0;
        if (s.mode == FVM_CRASH_ZIP || s.mode == FVM_CRASH_ZIP_CODE)
        {
            Bytes objects;
            heap(c, objects);
            Archive archive;
            const char manifest[] =
                "{\"format\":1,\"kind\":\"FlashVM VM fault\",\"byte_strings\":\"JSON escapes "
                "encode original bytes\",\"native_resources\":\"metadata "
                "only\",\"jit_backend\":\"see backtrace engine and host build\"}";
            archive.add("manifest.json", manifest, sizeof(manifest) - 1);
            archive.add("backtrace.json", s.report->trace.begin(), s.report->trace.size());
            archive.add("heap.json", objects.begin(), objects.size());
            for (size_t i = 0; i < s.report->frames.size(); ++i)
            {
                const SavedFrame &f = s.report->frames[i];
                char name[64];
                std::sprintf(name, "bytecode/frame%lu.avm1", (unsigned long)i);
                archive.add(name, f.bytecode.begin(), f.bytecode.size());
                if (!f.machine.empty())
                {
                    std::sprintf(name, "jit/frame%lu.bin", (unsigned long)i);
                    archive.add(name, f.machine.begin(), f.machine.size());
                }
            }
            archive.finish(s.report->zip);
            if (!s.path.empty())
            {
                FILE *file = std::fopen(s.path.c_str(), "wb");
                if (!file)
                    s.diagnostic_status = FVM_ERROR;
                else
                {
                    bool ok = std::fwrite(s.report->zip.begin(), 1, s.report->zip.size(), file) ==
                              s.report->zip.size();
                    if (std::fclose(file) != 0)
                        ok = false;
                    if (!ok)
                        s.diagnostic_status = FVM_ERROR;
                }
            }
        }
    }
    catch (const std::bad_alloc &)
    {
        delete r;
        s.diagnostic_status = FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        delete r;
        s.diagnostic_status = FVM_ERROR;
    }
    if (s.mode == FVM_CRASH_DEBUGGER && s.handler)
    {
        try
        {
            s.handler(c, status, s.userdata);
        }
        catch (...)
        {
            s.diagnostic_status = FVM_ERROR;
        }
    }
    s.busy = false;
}
/* Publish a fully copied policy before replacing the old path. */
extern "C" FVM_Result fvm_crash_configure(FVM_Context *c, FVM_CrashMode mode, const char *path,
                                          FVM_CrashHandler handler, void *data)
{
    if (!c || c->destroying || c->gc_releasing || c->avm1_execution_depth || c->crash.active ||
        c->crash.busy || mode < FVM_CRASH_NONE || mode > FVM_CRASH_DEBUGGER || (!handler && data) ||
        (mode == FVM_CRASH_DEBUGGER && !handler))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string copied(path ? path : "");
        c->crash.path.swap(copied);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    c->crash.mode = mode;
    c->crash.handler = handler;
    c->crash.userdata = data;
    return FVM_OK;
}
/* Select preservation cost independently from the report/break action. */
extern "C" FVM_Result fvm_crash_set_detail(FVM_Context *c, FVM_CrashDetail detail)
{
    if (!c || c->destroying || c->gc_releasing || c->avm1_execution_depth || c->crash.busy ||
        c->crash.active || detail < FVM_CRASH_DETAIL_NATURAL || detail > FVM_CRASH_DETAIL_FORCE)
        return FVM_INVALID_ARGUMENT;
    c->crash.detail = detail;
    return FVM_OK;
}
/* Drop owned postmortem bytes while preserving configured policy. */
extern "C" FVM_Result fvm_crash_clear(FVM_Context *c)
{
    if (!c || c->crash.busy)
        return FVM_INVALID_ARGUMENT;
    delete c->crash.report;
    c->crash.report = 0;
    c->crash.diagnostic_status = FVM_OK;
    return FVM_OK;
}
/* Expose original status independently from capture/I/O failures. */
extern "C" FVM_Result fvm_crash_info(FVM_Context *c, size_t *count, FVM_Result *status,
                                     FVM_Result *diagnostic)
{
    if (!c || !count || !status || !diagnostic)
        return FVM_INVALID_ARGUMENT;
    *diagnostic = c->crash.diagnostic_status;
    if (!c->crash.report)
    {
        *count = 0;
        return FVM_NOT_FOUND;
    }
    *count = c->crash.report->frames.size();
    *status = c->crash.report->status;
    return FVM_OK;
}
/* Return stable views into copied, context-owned frame data. */
extern "C" FVM_Result fvm_crash_frame(FVM_Context *c, size_t index, FVM_CrashFrame *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.report || index >= c->crash.report->frames.size())
        return FVM_NOT_FOUND;
    const SavedFrame &f = c->crash.report->frames[index];
    FVM_CrashFrame v;
    v.function = f.name.c_str();
    v.function_size = f.name.size();
    v.source = f.source.c_str();
    v.source_size = f.source.size();
    v.offset = f.offset;
    v.line = f.line;
    v.jit = f.jit;
    v.forced = f.forced;
    v.argc = f.argc;
    v.bytecode = f.bytecode.begin();
    v.bytecode_size = f.bytecode.size();
    v.machine_address = f.machine_address;
    v.machine_code = f.machine.begin();
    v.machine_code_size = f.machine.size();
    *out = v;
    return FVM_OK;
}
/* Return copied postmortem payload; no source VM pointers are embedded as live handles. */
extern "C" FVM_Result fvm_crash_data(FVM_Context *c, int zip, const void **out, size_t *size)
{
    if (!c || !out || !size)
        return FVM_INVALID_ARGUMENT;
    if (!c->crash.report)
        return FVM_NOT_FOUND;
    const Bytes &b = zip ? c->crash.report->zip : c->crash.report->trace;
    *out = b.begin();
    *size = b.size();
    return FVM_OK;
}
namespace
{
/* Resolve a live frame only inside the synchronous DEBUGGER callback. */
FVM_CrashActivation *live(FVM_Context *c, size_t index)
{
    if (!c || !c->crash.busy || c->crash.mode != FVM_CRASH_DEBUGGER)
        return 0;
    FVM_CrashActivation *f = c->crash.active;
    while (f && index--)
        f = f->previous;
    return f;
}
} // namespace
/* Borrow an original supplied argument from the suspended invocation. */
extern "C" FVM_Result fvm_crash_argument(FVM_Context *c, size_t frame, unsigned int index,
                                         FVM_Value *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    FVM_CrashActivation *f = live(c, frame);
    if (!f || index >= f->argc)
        return FVM_NOT_FOUND;
    *out = f->argv[index];
    return FVM_OK;
}
/* Enumerate only materialized activation fields; never force getters or reconstruct optimized
 * state. */
extern "C" FVM_Result fvm_crash_local(FVM_Context *c, size_t frame, size_t index, const char **name,
                                      size_t *size, FVM_Value *out)
{
    if (!c || !name || !size || !out)
        return FVM_INVALID_ARGUMENT;
    FVM_CrashActivation *f = live(c, frame);
    if (!f || !f->activation)
        return FVM_NOT_FOUND;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = f->activation->properties.begin();
    while (i != f->activation->properties.end() && index)
    {
        --index;
        ++i;
    }
    if (i == f->activation->properties.end())
    {
        if (!f->forced || !f->function || !f->function->function2)
            return FVM_NOT_FOUND;
        for (size_t j = 0; j < f->function->parameters.size(); ++j)
        {
            unsigned int reg = f->function->parameter_registers[j];
            if (!reg || reg >= f->register_count)
                continue;
            if (index)
            {
                --index;
                continue;
            }
            *name = f->function->parameters[j].data();
            *size = f->function->parameters[j].size();
            *out = f->registers[reg];
            return FVM_OK;
        }
        return FVM_NOT_FOUND;
    }
    *name = i->first.data();
    *size = i->first.size();
    *out = i->second;
    return FVM_OK;
}
/* Expose already-maintained registers without adding per-instruction spills. */
extern "C" FVM_Result fvm_crash_register(FVM_Context *c, size_t frame, unsigned int index,
                                         FVM_Value *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    FVM_CrashActivation *f = live(c, frame);
    if (!f || index >= f->register_count)
        return FVM_NOT_FOUND;
    *out = f->registers[index];
    return FVM_OK;
}
