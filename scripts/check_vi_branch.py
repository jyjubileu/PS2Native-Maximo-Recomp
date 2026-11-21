#!/usr/bin/env python3
"""Build and check VI branch timing without a game, BIOS or vendor libraries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TESTS = ROOT / "tests"
REFERENCE = json.loads((TESTS / "vi_branch_expected.json").read_text(encoding="utf-8"))
EXE = ".exe" if os.name == "nt" else ""
FIXTURE_SHA = "e2e40456933986943e47e0242346bfba17aad4663116b3e5c4ae2f0af43edaaf"
SUITES = ("ps2viftest", "ps2vifunpacktest", "ps2vifdefertest", "ps2vuregtest",
          "ps2vucleanuptest", "ps2vu0inlinetest", "ps2gifrunawaytest",
          "ps2decodecachetest", "ps2drawsetuptest", "ps2displaylinestest")
JIT_FALLBACK = {"01560FA9AC98CFA4_00C0", "3C7B0CB2B9D2A4FE_0180"}


def run(command, *, env=None, capture=False, check=True):
    return subprocess.run(
        [str(item) for item in command], cwd=ROOT, env=env, check=check,
        capture_output=capture, text=True, encoding="utf-8", errors="replace",
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "clang++"))
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--ninja", default="ninja")
    parser.add_argument("--node", default=shutil.which("node"))
    parser.add_argument("--work", type=Path, help="New work directory (default: build/vi-*)")
    parser.add_argument("--jobs", type=int, default=6)
    args = parser.parse_args()

    if args.work:
        work = args.work.resolve()
        work.mkdir(parents=True, exist_ok=False)
    else:
        (ROOT / "build").mkdir(exist_ok=True)
        work = Path(tempfile.mkdtemp(prefix="vi-", dir=ROOT / "build"))
    build = work / "cmake"
    exe = lambda name: build / (name + EXE)
    base_env = {k: v for k, v in os.environ.items() if not k.startswith("PS2_")}
    base_env.update(PS2_EE_LIVE="0", PS2_VU_LIVE="0")
    clang = shutil.which(args.cxx) or args.cxx
    ninja = shutil.which(args.ninja) or args.ninja
    config = [
        args.cmake, "-S", ROOT, "-B", build, "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_CXX_COMPILER={Path(clang).as_posix()}",
        f"-DCMAKE_MAKE_PROGRAM={Path(ninja).as_posix()}",
        f"-DPS2_WGPU_DIR={(work / 'cpu-only').as_posix()}",
        "-DPS2_RUNTIME_LIB=", "-DPS2_GENERATED=", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    compile_args = [args.cmake, "--build", build, "--parallel", args.jobs, "--target"]
    run(config, env=base_env)
    run([*compile_args, "vurecomp", "vujit_dump", "ps2vibranchtest", *SUITES], env=base_env)
    records = work / "recorded"
    records.mkdir()
    readme_oracle = work / "fixtures"
    output = {}

    def native(name, mode):
        fixture = readme_oracle / name
        rows = REFERENCE[name]["qwords"]
        env = base_env.copy()
        if mode == "exact":
            env["PS2_VU_EXACT"] = "1"
        elif mode == "fast":
            env["PS2_VU_INTERPRET"] = "1"
        elif mode == "record":
            env["PS2_VU_RECORD"] = str(records)
        result = run([exe("ps2vibranchtest"), fixture / "minimo-vif1.bin", *rows], env=env, capture=True, check=False)
        expected = "".join(f"{pc} {value}\n" for pc, value in rows.items())
        if result.returncode or result.stdout != expected:
            raise RuntimeError(f"{name}/{mode} differs\nExpected:\n{expected}Actual:\n{result.stdout}\n{result.stderr}")
        m = re.search(r"fast_interpreter_calls=(\d+)", result.stderr)
        if not m or (mode == "aot" and int(m[1])) or (mode in ("record", "fast") and not int(m[1])):
            raise RuntimeError(f"{name}/{mode} did not use the requested path\n{result.stderr}")
        output.setdefault(name, {})[mode] = int(m[1])
        print(f"PASS {name}/{mode} ({len(rows)} qwords, {m[1]} fast entry calls)", flush=True)

    for name, spec in REFERENCE.items():
        fixture = readme_oracle / name
        option = [spec["option"]] if spec["option"] else []
        run([sys.executable, TESTS / "vi_branch_fixture.py", fixture, *option])
        stream = fixture / "minimo-vif1.bin"
        if name == "minimal" and (stream.stat().st_size != 1664 or hashlib.sha256(stream.read_bytes()).hexdigest() != FIXTURE_SHA):
            raise RuntimeError("The minimal stream differs from the Stage 3 fixture")
        for mode in ("exact", "fast", "record"):
            native(name, mode)
    generated = work / "generated"
    run([exe("vurecomp"), records, generated], env=base_env)
    run([*config, f"-DPS2_VI_TEST_GENERATED={generated.as_posix()}"], env=base_env)
    run([*compile_args, "ps2vibranchtest"], env=base_env)
    for name in REFERENCE:
        native(name, "aot")

    wasm_status = "skipped (Node.js unavailable)"
    if args.node:
        wasm = work / "wasm"
        result = run([exe("vujit_dump"), records, wasm], env=base_env, check=False)
        recorded = {p.stem for p in records.glob("*.bin")}
        emitted = {p.stem for p in wasm.glob("*.wasm")}
        if result.returncode != 1 or len(emitted) != 16 or recorded - emitted != JIT_FALLBACK:
            raise RuntimeError("Unexpected JIT fallback set")
        for name, spec in REFERENCE.items():
            if spec["wasm_entries"] is None:
                continue
            rows = {p: v for p, v in spec["qwords"].items() if p != "045"}  # 045 is a VIF write
            expected = readme_oracle / name / "expected-wasm.txt"
            expected.write_text("".join(f"{p} {v}\n" for p, v in rows.items()), encoding="utf-8")
            run([args.node, TESTS / "vi_branch_wasm.mjs", wasm, records,
                 readme_oracle / name / "minimo-vu1.bin", expected, spec["wasm_entries"], *rows], env=base_env)
            print(f"PASS {name}/wasm", flush=True)
        wasm_status = "passed; 16 modules emitted, two indirect roots use interpreter"

    for target in SUITES:
        run([exe(target)], env=base_env)
    run([exe("ps2vifdefertest"), "off"], env=base_env)
    run([exe("ps2vuregtest")], env=base_env | {"PS2_VU_EXACT": "1"})
    tools_env = base_env | {"PATH": f"{Path(clang).resolve().parent}{os.pathsep}{base_env.get('PATH', '')}"}
    run([sys.executable, TESTS / "vurecomp_incremental.py", "--vurecomp", exe("vurecomp")], env=tools_env)
    summary = {"cases": output, "wasm": wasm_status, "suites": [*SUITES, "ps2vifdefertest off",
               "ps2vuregtest exact", "vurecomp_incremental.py"], "clean_cpu_build": True}
    (work / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print("VI differential checks and native suites passed.", flush=True)


if __name__ == "__main__":
    main()
