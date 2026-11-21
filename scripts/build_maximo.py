#!/usr/bin/env python3
"""Build the PAL Maximo runner with only local disc/patch data and current sources."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SERIAL = "SLES_517.59"
CRC = "86E533E0"
SCRIPTS = ROOT / "scripts"
EXE = ".exe" if os.name == "nt" else ""


def run(args, *, capture=False):
    command = [str(x) for x in args]
    print(subprocess.list2cmdline(command) if os.name == "nt" else shlex.join(command), flush=True)
    return subprocess.run(command, cwd=ROOT, check=True, text=True, encoding="utf-8",
                          errors="replace", capture_output=capture)


def python_script(name, *args, capture=False):
    return run([sys.executable, "-X", "utf8", "-I", SCRIPTS / name, *args], capture=capture)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--disc", required=True, type=Path, help="Local PAL ISO from your own disc")
    p.add_argument("--profile", required=True, choices=("4x3", "16x9"))
    p.add_argument("--pnach", type=Path, action="append", default=[], help="Local patch file (repeatable)")
    p.add_argument("--section", action="append", default=[], help="Optional named PNACH section (repeatable)")
    p.add_argument("--seeds", type=Path, help="Optional local entries.txt, overlays/, vu0/ and vu1/")
    p.add_argument("--work", type=Path, help="New directory (default: build/maximo-pal-<profile>)")
    p.add_argument("--cxx", default=os.environ.get("CXX", "clang++"))
    p.add_argument("--cmake", default="cmake")
    p.add_argument("--ninja", default="ninja")
    p.add_argument("--wgpu", type=Path, default=ROOT / "third_party" / "wgpu-native")
    p.add_argument("--sdl2", type=Path)
    p.add_argument("--jobs", type=int, default=6)
    args = p.parse_args()
    if args.profile == "16x9" and not args.pnach:
        p.error("The audited PAL widescreen profile requires a local PNACH")
    if args.section and not args.pnach:
        p.error("--section selects a section of --pnach")
    iso = args.disc.resolve(strict=True)
    if not iso.is_file():
        p.error("--disc must be a local ISO file")
    patches = [x.resolve(strict=True) for x in args.pnach]
    seeds = args.seeds.resolve(strict=True) if args.seeds else None
    work = (args.work or ROOT / "build" / ("maximo-pal-" + args.profile)).resolve()
    work.mkdir(parents=True, exist_ok=False)
    generated = work / "generated"
    tools = work / "tools"
    native = work / "native"
    game = work / "game"
    disc = game / "disc"
    disc.mkdir(parents=True)
    python_script("disc.py", "extract", iso, disc, "SYSTEM.CNF", SERIAL)
    config = (disc / "SYSTEM.CNF").read_bytes()
    boot_line = rb"(?mi)^[ \t]*BOOT2[ \t]*=[ \t]*cdrom0:[\\/]+" + re.escape(SERIAL.encode()) + rb"(?:;1)?[ \t\r]*$"
    if not re.search(boot_line, config):
        raise RuntimeError(f"SYSTEM.CNF must boot the PAL {SERIAL} executable")
    elf = disc / SERIAL
    detected_crc = python_script("pnach.py", "crc", elf, capture=True).stdout.strip()
    if detected_crc.upper() != CRC:
        raise RuntimeError(f"Unexpected ELF CRC {detected_crc}; the audited PAL CRC is {CRC}")

    run_options = []
    if patches:
        output = game / "patched"
        output.mkdir()
        selected = args.section or (["Widescreen 16:9"] if args.profile == "16x9" else [])
        section_flags = [item for name in selected for item in ("--section", name)]
        patched_elf = output / SERIAL
        runtime_patches = game / "patches.txt"
        python_script("pnach.py", "apply", elf, patched_elf, runtime_patches, "--crc", CRC, *section_flags, *patches)
        elf = patched_elf
        run_options.extend(("--patches", str(runtime_patches)))
    if args.profile == "16x9":
        run_options.append("--widescreen")
    toggles = game / "toggles.txt"
    if toggles.is_file():
        run_options.extend(("--toggles", str(toggles)))

    cxx = Path(shutil.which(args.cxx) or args.cxx).resolve(strict=True)
    ninja = Path(shutil.which(args.ninja) or args.ninja).resolve(strict=True)
    config = [
        args.cmake, "-S", ROOT, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_CXX_COMPILER={cxx.as_posix()}",
        f"-DCMAKE_MAKE_PROGRAM={ninja.as_posix()}",
        f"-DPS2_WGPU_DIR={args.wgpu.resolve().as_posix()}",
        "-DPS2_RUNTIME_LIB=", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    if args.sdl2:
        config.append(f"-DSDL2_DIR={args.sdl2.resolve(strict=True).as_posix()}")
    run([*config, "-B", tools, "-DPS2_GENERATED="])
    run([args.cmake, "--build", tools, "--parallel", args.jobs, "--target", "eerecomp", "vurecomp"])

    entries = game / "entries.txt"
    if seeds and (seeds / "entries.txt").is_file():
        shutil.copyfile(seeds / "entries.txt", entries)
    else:
        entries.touch()
    translate_options = ["--entries", entries]
    if seeds and (seeds / "overlays").is_dir():
        translate_options.extend(("--overlays", seeds / "overlays"))
    if toggles.is_file():
        translate_options.extend(("--toggles", toggles))
    if any(signature in elf.read_bytes() for signature in (b"SNR2", b"xff2")):
        modules = game / "modules"
        python_script("disc.py", "modules", iso, modules)
        translate_options.extend(("--modules", modules))
    generated.mkdir()
    run([tools / ("eerecomp" + EXE), elf, *translate_options, "--out", generated])

    if seeds:
        vu1, vu0 = seeds / "vu1", seeds / "vu0"
        if any(vu1.glob("*.bin")) or any(vu0.glob("*.bin")):
            empty = game / "empty-vu-recordings"
            empty.mkdir()
            run([tools / ("vurecomp" + EXE), vu1 if vu1.is_dir() else empty, generated,
                 "--vu0-dir", vu0 if vu0.is_dir() else empty])

    run([*config, "-B", native, f"-DPS2_GENERATED={generated.as_posix()}"])
    run([args.cmake, "--build", native, "--parallel", args.jobs, "--target", "ps2run"])
    command = [str(native / ("ps2run" + EXE)), str(elf), "--disc", str(iso),
               "--game-dir", str(game), *run_options]
    (work / "runner.json").write_text(json.dumps(command, indent=2) + "\n", encoding="utf-8")
    print("Play:", subprocess.list2cmdline([*command, "--window"]) if os.name == "nt"
          else shlex.join([*command, "--window"]))


if __name__ == "__main__":
    main()
