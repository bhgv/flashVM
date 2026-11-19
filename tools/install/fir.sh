# Copyright (c) 2026 bhgv
# EN: FIR compiler CLI, libraries and teaching examples.
# RU: Упаковка продуктов без исполнения целевых программ.


# EN: Borrow install_component locals (base/destination/component/source) through Bash scope;
# use shared copy/program helpers. Copy failures propagate through set -e; no owned inputs.
# RU: Заимствовать base/destination/component/source из install_component через область Bash;
# использовать общие copy/program. Ошибки передаются через set -e, входы не поглощаются.
install_fir_package() {
    source="$ROOT/fir/v1"
    mkdir -p "$destination/examples"
    for file in fir1 fir1_source fir1_opt fir1_table_run fir1_integer_identities_example; do
        install_program "$base" "$file" "$destination"
    done
    install_program "$base" fir1_dis_example "$destination"
    copy_tree "$ROOT/examples/firststeps" "$destination/examples/firststeps"
    copy_tree "$source/examples/dis" "$destination/examples/dis"
    copy_tree "$source/examples/tables" "$destination/examples/tables"
    copy_tree "$source/examples/type-aliases" "$destination/examples/type-aliases"
    copy_tree "$source/examples/integer-identities" "$destination/examples/integer-identities"
    copy_tree "$source/include" "$destination/include"
    copy_file "$source/compiler/dis.c" "$destination/source/dis.c"
    copy_file "$source/compiler/dis_table.c" "$destination/source/dis_table.c"
    for file in DIVISION_FACTS.md DIVISION_FACTS.ru.md SPARSE_PRODUCT_FACTS.md SPARSE_PRODUCT_FACTS.ru.md INTEGER_PAIRS.md INTEGER_PAIRS.ru.md INTEGER_FACTS.md INTEGER_FACTS.ru.md SHIFT_FACTS.md SHIFT_FACTS.ru.md KNOWN_RESULTS.md KNOWN_RESULTS.ru.md SWF_INT_DIVMOD.md SWF_INT_DIVMOD.ru.md SWF_INT_SHIFTS.md SWF_INT_SHIFTS.ru.md INTEGER_IDENTITIES.md INTEGER_IDENTITIES.ru.md NUMERIC_FLOW.md NUMERIC_FLOW.ru.md NUMERIC_FOR.md NUMERIC_FOR.ru.md GROUPED_CONDITIONS.md GROUPED_CONDITIONS.ru.md NUMERIC_LOGIC.md NUMERIC_LOGIC.ru.md REAL_CONDITIONS.md REAL_CONDITIONS.ru.md COVERAGE.md COVERAGE.ru.md TYPED_PREDICATES.md TYPED_PREDICATES.ru.md INT_BITS.md INT_BITS.ru.md SOURCE_ALIASES.md SOURCE_ALIASES.ru.md DIS_BACKEND.md DIS_BACKEND.ru.md TABLES.md TABLES.ru.md TABLES_DIS.md TABLES_DIS.ru.md EXTENSIONS.md EXTENSIONS.ru.md TYPE_REFLECTION.md TYPE_REFLECTION.ru.md BIG_ARITHMETIC.md BIG_ARITHMETIC.ru.md BIG_BITWISE.md BIG_BITWISE.ru.md BIG_CFG.md BIG_CFG.ru.md BIG_DECISIONS.md BIG_DECISIONS.ru.md TABLE_CFG.md TABLE_CFG.ru.md TABLE_MUTATION.md TABLE_MUTATION.ru.md TERMINAL_ARMS.md TERMINAL_ARMS.ru.md COMPARISON_VALUES.md COMPARISON_VALUES.ru.md SHORT_CIRCUIT.md SHORT_CIRCUIT.ru.md COMPOUND_UPDATES.md COMPOUND_UPDATES.ru.md UNICODE_STRINGS.md UNICODE_STRINGS.ru.md INT_ARITHMETIC.md INT_ARITHMETIC.ru.md TEXT_BITS.md TEXT_BITS.ru.md INT_SHIFTS.md INT_SHIFTS.ru.md TEXT_VALUES.md TEXT_VALUES.ru.md TABLE_LITERALS.md TABLE_LITERALS.ru.md RELATIVE_SEQUENCES.md RELATIVE_SEQUENCES.ru.md SLICE_ASSIGNMENT.md SLICE_ASSIGNMENT.ru.md BIG_SHIFTS.md BIG_SHIFTS.ru.md; do
        copy_file "$source/doc/$file" "$destination/docs/$file"
    done
    for file in API.md API.ru.md; do
        copy_file "$source/compiler/$file" "$destination/docs/$file"
    done
    "${PYTHON:-python3}" "$ROOT/tools/link-installed-fir-docs.py" "$ROOT" "$destination"
    cat >"$destination/README-FIRSTSTEPS.md" <<'EOF'
# Numeric source -> FIR -> object

./fir1_source as2 --input examples/firststeps/arithmetic.as arithmetic.fir
./fir1 check arithmetic.fir
./fir1 opt arithmetic.fir arithmetic.opt.fir
./fir1 object arithmetic.opt.fir arithmetic.swo

Pass arithmetic.swo to the separately packaged swl linker. The included source,
expected FIR and SWF illustrate the numeric subset, not a complete AS compiler.
If your host restricts temporary files, set TMPDIR to an existing writable folder.

Typed tables: [source/FIR](docs/TABLES.md), [native Dis](docs/TABLES_DIS.md),
[русский контракт](docs/TABLES_DIS.ru.md). Compile examples/tables/main.b with
fir1_source, then fir1 dis tables.fir tables.dis and run it with the matching
hosted disvm --jit --quantum 1 --inspect-words 0 2 tables.dis. Expected: 42, 1.
The reference runner and C99 reference/compiler clients remain available separately.

# Our Limbo numeric subset -> direct NC64 Dis (no limbo-native/assembler)
./fir1_source limbo --input examples/dis/main.b answer.fir
./fir1 dis answer.fir answer.dis
# Run with the separately installed NC64 DisVM:
../disvm-editor/disvm --jit --inspect-words 0 1 answer.dis
# Expected: DATA 0 2147483647

Read examples/dis/README.md and README.ru.md, plus docs/DIS_BACKEND.md/.ru.md.
The target captures i32/bool/f64 trace values plus integer executed flags after completion.
It supports verified CFG and terminal Limbo decisions, not full module lowering. The C99 example and libfir1_ir/libfir1_source_api/libfir1_io are included.

# Terminal decision: values [0,42,0], executed flags [0,1,0].
./fir1_source limbo --input examples/dis/decision.b decision.fir
./fir1 dis decision.fir decision.dis
../disvm-editor/disvm --jit --max-instructions 1000 --inspect-words 0 6 decision.dis

# Numeric AS3 binary64: -11.5, 3.75, -1.25; flags at offsets 24/32/40.
./fir1_source as3 --input examples/dis/reals.as reals.fir
./fir1 dis reals.fir reals.dis
../disvm-editor/disvm --jit --inspect-reals 0 3 reals.dis

# Homogeneous Limbo real: explicit real return type, result 42.5.
./fir1_source limbo --input examples/dis/real.b real.fir
./fir1 dis real.fir real.dis
../disvm-editor/disvm --jit --inspect-reals 0 1 real.dis
EOF
}
