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
        for variant in range(25):
            main_source = work / "main.b"
            template_name = {10: "kill.b", 16: "child.b", 17: "child.b", 18: "child.b", 19: "child.b", 24: "contract-child.b"}.get(variant, "main.b")
            template = (fixtures / template_name).read_text()
            expected = {0: 111, 16: 111, 17: 222, 18: 111, 19: 222}.get(variant, -1)
            if variant == 13:
                template = template.replace("app == nil || app->value() != EXPECTED", "app != nil")
            if variant in (20, 21, 22, 23):
                template = template.replace("if (app == nil || app->value() != EXPECTED) {",
                                            "if (app != nil) {\n        ignored := app->value();")
            main_source.write_text(template.replace("EXPECTED", str(expected))
                                   .replace("HELPERS", "" if variant else (fixtures / "update.b").read_text())
                                   .replace("SWITCH", "" if variant else "update(sys, app);"))
            compile_image(compiler, include, root, main_source, work / "main.dis", [fixtures])
            binary = relink(runtime, work, root / "tests/provider_modules/launch.c", variant,
                            ["dvm_runtime_launch_module", "dvm_namespace_boot_shutdown",
                             "dvm_launch_process_adopt", "dvm_resource_plan_call",
                             "dvm_launch_module_checked", "malloc", "calloc", "free"])
            for mode in ["--no-jit", "--jit"]:
                result = subprocess.run([binary, "--native-spawn", mode, work / "main.dis"], cwd=work,
                                        capture_output=True, text=True, timeout=30)
                if result.returncode or "PASS provider-backed Dis module and dependency" not in result.stdout or \
                        "PASS provider module source selection and native ownership drain" not in result.stdout:
                    raise RuntimeError(f"provider module variant={variant} {mode}\n" + result.stdout + result.stderr)
                print(f"PASS provider module variant={variant} {mode}", flush=True)


if __name__ == "__main__":
    main()
