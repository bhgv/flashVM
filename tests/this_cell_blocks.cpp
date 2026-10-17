/* With/Catch copy this, while Try/Finally mutate the enclosing activation cell. */
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include "this_cell_assignment.cpp"
static void cell_word(Program &p, size_t value) { p.op((unsigned char)value); p.op((unsigned char)(value >> 8)); }
static void cell_assign(Program &p, unsigned value) { p.text("this"); p.number(value); p.op(0x1d); }
static void cell_observe(Program &p, const char *name) { p.text(name); p.variable("this"); p.op(0x1d); }
static void cell_append(Program &p, const Program &body) { for (size_t i = 0; i < body.bytes.size(); ++i) p.op(body.bytes[i]); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
    for (unsigned mode = 0; mode < 3; ++mode)
    for (unsigned kind = 0; kind < 4; ++kind) {
        if (mode && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *owner = 0; assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK); FVM_Value value = class_value(owner); assert(fvm_set_global(c, "owner", &value) == FVM_OK);
        Program outer; cell_assign(outer, 17);
        if (!kind) {
            Program block; cell_observe(block, "before"); cell_assign(block, 27); cell_observe(block, "inside");
            outer.variable("owner"); outer.op(0x94); cell_word(outer, 2); cell_word(outer, block.bytes.size()); cell_append(outer, block);
        } else {
            Program body, caught, final, header;
            cell_assign(body, 27); if (kind >= 2) { body.number(91); body.op(0x2a); }
            cell_observe(caught, "before"); cell_assign(caught, 37); cell_observe(caught, "inside");
            cell_observe(final, "finalBefore"); cell_assign(final, 47);
            header.op(kind == 1 ? 2 : 3); cell_word(header, body.bytes.size()); cell_word(header, kind >= 2 ? caught.bytes.size() : 0); cell_word(header, final.bytes.size());
            const char *name = kind == 3 ? "this" : "error"; for (size_t i = 0; i <= std::strlen(name); ++i) header.op((unsigned char)name[i]);
            outer.op(0x8f); cell_word(outer, header.bytes.size()); cell_append(outer, header); cell_append(outer, body); if (kind >= 2) cell_append(outer, caught); cell_append(outer, final);
        }
        outer.variable("this"); outer.finish(); value = boxing_run(c, outer, mode, FVM_OK);
        assert(value.type == FVM_VALUE_NUMBER && value.number == (!kind ? 17 : 47));
        if (!kind || kind >= 2) { assert(fvm_get_global(c, "before", &value) == FVM_OK && value.number == (!kind ? 17 : kind == 3 ? 91 : 27)); assert(fvm_get_global(c, "inside", &value) == FVM_OK && value.number == (!kind ? 27 : 37)); }
        if (kind) assert(fvm_get_global(c, "finalBefore", &value) == FVM_OK && value.number == 27);
        assert(!c->script_this_cell && !c->script_this && !c->gc_frames && !c->script_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
