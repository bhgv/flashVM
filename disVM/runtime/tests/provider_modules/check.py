# Copyright (c) 2026 bhgv
# EN: Real native-cell Dis loads, dependency source priority, malformed/denied input and same-path live updates.
# RU: Настоящие native-cell Dis loads, приоритет зависимостей, malformed/denied input и live-обновления одного пути.
from pathlib import Path
import subprocess
import sys
import tempfile

# EN/RU: Reuse repository-owned native compiler/relink helpers from this explicitly located runtime.
# Использовать native compiler/relink helpers репозитория из явно заданного runtime.
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile portable dependencies and bootstrap, run actual interpreter/JIT with independent provider failure variants.
# Собрать переносимые зависимости/bootstrap и выполнить настоящий interpreter/JIT с вариантами отказа provider.
def main():
    runtime, compiler, include = [Path(value).resolve() for value in sys.argv[1:4]]
    root = Path(__file__).resolve().parents[2]
    fixtures = root / "fixtures/provider-modules"
    with tempfile.TemporaryDirectory(prefix="provider-modules-") as name:
        work = Path(name)
        for label, value in [("first", 111), ("next", 112), ("terminal", 222)]:
            source = work / "value.b"
            source.write_text((fixtures / "constant.b").read_text().replace("EXPECTED_VALUE", str(value)))
            compile_image(compiler, include, root, source, work / "value.dis", [fixtures])
            (work / (label + ".dis")).write_bytes((work / "value.dis").read_bytes())
        compile_image(compiler, include, root, fixtures / "app.b", work / "app.dis", [fixtures])
        compile_image(compiler, include, root, fixtures / "wrong.b", work / "wrong.dis", [fixtures])
        selected = [int(value) for value in sys.argv[4:]] or list(range(43))
        if any(value < 0 or value > 42 for value in selected):
            raise ValueError("provider module variant outside 0..42")
        for variant in selected:
            if variant >= 30:
                dependency = "dependency-child.b" if variant == 31 else "dependency-context.b"
                for label, value in [("first", 111), ("next", 112)]:
                    source = work / "value.b"
                    source.write_text((fixtures / dependency).read_text().replace("EXPECTED_VALUE", str(value)))
                    compile_image(compiler, include, root, source, work / (label + ".dis"), [fixtures])
            if variant >= 25:
                app_source = (fixtures / "app.b").read_text().replace('include "value.m";', 'include "value.m";\ninclude "sys.m";')
                extra = '    sys := load Sys Sys->PATH;\n'
                if variant == 26:
                    extra += '    if (sys->pctl(Sys->FORKNS, nil) < 0) { raise "module FORKNS failed"; }\n'
                if variant == 27:
                    extra += '    if (sys->open("/saved", Sys->OREAD) != nil || sys->bind("/data", "/saved", 0) < 0) { raise "module instances share namespace edits"; }\n'
                if variant == 28:
                    extra += '    input := array[4] of byte;\n    if (sys->read(sys->fildes(0), input, 4) != 4 || string input != "left") { raise "module lost redirected stdin"; }\n'
                extra += '    data := sys->open("/data", Sys->OREAD);\n    bytes := array[32] of byte;\n    n := sys->read(data, bytes, len bytes);\n    if (n != 9 || string bytes[:n] != "execution") { raise "module builtin lost owner-first environment"; }\n'
                app_source = app_source.replace('    dep := load Value', extra + '    dep := load Value')
                if variant >= 29:
                    extra_result = "int arguments[2]" if variant == 32 else "0"
                    app_source = (fixtures / "app-context.b").read_text().replace("RESULT_EXTRA", extra_result)
                if variant >= 33:
                    mode = variant - 33 if variant <= 36 else 3 if variant == 38 else 0
                    app_source = (fixtures / "app-namespace-child.b").read_text().replace("TEST_MODE", str(mode))
                if variant >= 39:
                    app_source = (fixtures / "app-events.b").read_text()
                    if variant == 42:
                        check = ('        if (sys->pctl(Sys->FORKNS, nil) >= 0) {\n'
                                 '            raise "scope subscription refusal missing";\n        }\n')
                        app_source = app_source.replace('        if (sys->pctl', check + '        if (sys->pctl', 1)
                source = work / "app.b"
                source.write_text(app_source)
                compile_image(compiler, include, root, source, work / "app.dis", [fixtures])
            main_source = work / "main.b"
            template_name = {10: "kill.b", 16: "child.b", 17: "child.b", 18: "child.b", 19: "child.b", 24: "contract-child.b"}.get(variant, "main.b")
            template = (fixtures / template_name).read_text()
            expected = {0: 111, 16: 111, 17: 222, 18: 111, 19: 222, 25: 111, 26: 111, 27: 111, 28: 111, 29: 111, 30: 111, 31: 111, 32: 112, 33: 111, 34: 111, 35: 111, 36: 111, 37: 111, 38: 111, 39: 111, 40: 111, 41: 111, 42: 111}.get(variant, -1)
            if variant == 13:
                template = template.replace("app == nil || app->value() != EXPECTED", "app != nil")
            if variant in (20, 21, 22, 23):
                template = template.replace("if (app == nil || app->value() != EXPECTED) {",
                                            "if (app != nil) {\n        ignored := app->value();")
            if variant == 27:
                template = template.replace('    data := sys->open', '    other := load Value "/app.dis";\n    if (other == nil || other->value() != 111 || sys->open("/saved", Sys->OREAD) != nil) { raise "module namespace escaped to caller"; }\n    data := sys->open')
            if variant == 28:
                template = template.replace('    SWITCH', '    input := array[4] of byte;\n    if (sys->read(sys->fildes(0), input, 4) != 4 || string input != "next") { raise "redirected stdin offset changed"; }\n    SWITCH')
            if variant >= 29:
                check_context = '    arguments = launch->arguments();\n    if (len arguments != 4 || int arguments[0] != 0) { raise "module metadata escaped into caller"; }\n'
                if variant == 32:
                    check_context += '    other := load Value "/app.dis";\n    if (other == nil || other->value() != 113) { raise "module instances shared launch context"; }\n'
                if variant >= 33:
                    check_context += '    if (sys->open("/private", Sys->OREAD) != nil) { raise "private module namespace escaped to caller"; }\n'
                template = template.replace('    data := sys->open', check_context + '    data := sys->open')
            if variant >= 39:
                setup = ('    caller_events := sys->open("/dev/sysevents", Sys->OREAD);\n'
                         '    event_bytes := array[16384] of byte;\n'
                         '    if (caller_events == nil) { raise "caller event grant missing"; }\n'
                         '    while (sys->read(caller_events, event_bytes, len event_bytes) > 0) {}\n')
                template = template.replace('    app := load Value', setup + '    app := load Value')
                checks = ('    if (sys->read(caller_events, event_bytes, len event_bytes) >= 0) { raise "module events escaped into caller"; }\n'
                          '    if (sys->bind("/data", "/calleronly", 0) < 0) { raise "caller bind failed"; }\n'
                          '    if (app->value() != 111) { raise "module event reentry failed"; }\n'
                          '    if (sys->read(caller_events, event_bytes, len event_bytes) <= 0) { raise "caller event scope lost"; }\n')
                if variant == 40:
                    checks += ('    other := load Value "/app.dis";\n'
                               '    if (other == nil || other->value() != 111 || other->value() != 111) { raise "independent module event scope failed"; }\n')
                if variant == 41:
                    checks = ('    refused := 0;\n    {\n' + checks +
                              '    } exception {\n        "module events escaped into caller" => refused = 1;\n    }\n'
                              '    if (!refused) { raise "wrong inbox control escaped detection"; }\n'
                              '    sys->print("REJECTED module event scope confusion\\n");\n')
                template = template.replace('    data := sys->open', checks + '    data := sys->open')
            if variant == 37:
                checked_call = '    if (app == nil || app->value() != EXPECTED) {\n        raise "provider application dependency mismatch";\n    }'
                refusal = ('    refused := 0;\n    {\n        ignored := app->value();\n'
                           '    } exception {\n        "bound child lost private namespace" => refused = 1;\n    }\n'
                           '    if (!refused) { raise "namespace inheritance omission escaped detection"; }\n'
                           '    sys->print("REJECTED namespace inheritance omission\\n");')
                if template.count(checked_call) != 1:
                    raise RuntimeError("namespace omission control source anchor drift")
                template = template.replace(checked_call, refusal)
            main_source.write_text(template.replace("EXPECTED", str(expected))
                                   .replace("HELPERS", "" if variant else (fixtures / "update.b").read_text())
                                   .replace("SWITCH", "" if variant else "update(sys, app);"))
            compile_image(compiler, include, root, main_source, work / "main.dis", [fixtures])
            binary = relink(runtime, work, root / "tests/provider_modules/launch.c", variant,
                            ["dvm_runtime_launch_module", "dvm_namespace_boot_shutdown",
                             "dvm_launch_process_adopt", "dvm_resource_plan_call",
                             "dvm_launch_module_checked", "dvm_sysevents_group", "dvm_sysevents_open", "Sys_bind",
                             "dvm_module_environment_inherit", "dvm_launch_process_inherit", "linkmod", "unload", "malloc", "calloc", "free"])
            for mode in ["--no-jit", "--jit"]:
                result = subprocess.run([binary, "--native-spawn", mode, work / "main.dis"], cwd=work,
                                        capture_output=True, text=True, input="leftnext", timeout=30)
                if variant == 41:
                    if result.returncode or "REJECTED module event scope confusion" not in result.stdout or \
                            "PASS provider module source selection and native ownership drain" not in result.stdout:
                        raise RuntimeError(f"wrong inbox control missed semantic failure {mode}\n" + result.stdout + result.stderr)
                    print(f"PASS wrong module event inbox rejected {mode}", flush=True)
                    continue
                if variant == 37:
                    if result.returncode or "REJECTED namespace inheritance omission" not in result.stdout or \
                            "PASS provider module source selection and native ownership drain" not in result.stdout:
                        raise RuntimeError(f"namespace inheritance omission control missed semantic failure {mode}\n" + result.stdout + result.stderr)
                    print(f"PASS namespace inheritance omission rejected {mode}", flush=True)
                    continue
                if result.returncode or "PASS provider-backed Dis module and dependency" not in result.stdout or \
                        "PASS provider module source selection and native ownership drain" not in result.stdout:
                    raise RuntimeError(f"provider module variant={variant} {mode}\n" + result.stdout + result.stderr)
                print(f"PASS provider module variant={variant} {mode}", flush=True)


if __name__ == "__main__":
    main()
