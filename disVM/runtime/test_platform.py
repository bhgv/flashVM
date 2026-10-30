"""Validate CPU aliases and hard rejection paths independently of the build host."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Select only the pure platform function. These checks validate configuration,
# not native code execution; emitted ARM instructions require separate VM runs.
def main():
    cmake, selector = sys.argv[1:3]
    with tempfile.TemporaryDirectory(prefix="runtime platform ") as directory:
        script = Path(directory) / "select.cmake"
        selector = str(Path(selector).resolve()).replace("\\", "/").replace('"', '\\"')
        script.write_text(
            f'include("{selector}")\n'
            'dvm_runtime_select_backend("${OS}" "${CPU}" "${BYTES}" selected)\n'
            'message(STATUS "SELECTED=${selected}")\n'
        )
        cases = [
            ("Linux", "x86_64", 8, "amd64"),
            ("Linux", "AMD64", 8, "amd64"),
            ("Linux", "aarch64", 8, "arm64"),
            ("Linux", "ARM64", 8, "arm64"),
            ("Linux", "AArch64", 8, "arm64"),
            ("Linux", "riscv64", 8, "riscv64"),
            ("Linux", "RV64", 8, "riscv64"),
            ("Linux", "i686", 4, "64-bit userspace"),
            ("Linux", "aarch64", 4, "64-bit userspace"),
            ("Linux", "armv7l", 4, "64-bit userspace"),
            ("Linux", "mips64", 8, "no selected backend"),
            ("Darwin", "arm64", 8, "requires Linux"),
        ]
        for os_name, cpu, size, expected in cases:
            result = subprocess.run(
                [cmake, f"-DOS={os_name}", f"-DCPU={cpu}", f"-DBYTES={size}", "-P", str(script)],
                capture_output=True, text=True, check=False,
            )
            success = expected in ("amd64", "arm64", "riscv64")
            output = result.stdout + result.stderr
            if (result.returncode == 0) != success or (f"SELECTED={expected}" if success else expected) not in output:
                raise RuntimeError(f"platform admission {os_name}/{cpu}/{size}: {output}")
    print("PASS runtime platform selection: Linux64 AMD64/ARM64/RV64 aliases and explicit refusal paths")


if __name__ == "__main__":
    main()
