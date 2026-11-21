# Publication audit

## Source tree

The local tree contains the PS2Native launcher, CMake build, EE and VU translators, the runtime, web sources, upstream disc tools, and synthetic tests. Only original source code under its documented license, text documentation, and synthetic expected results are intended for Git. The [provenance record](provenance.md) identifies the upstream marker, the 19-file local baseline and the sealed Stage 4 arch files.

Commercial discs, game ELFs, per-game PNACH files, captured microcode and EE overlays, generated C++, runners, libraries, text logs with machine paths, snapshots, saves, memory cards, recorded media and downloaded dependency binaries are local inputs or outputs. The imported upstream CI uses public synthetic tests and skips its default jobs in a private repository. No credentials or game binaries are required by the VI test script.

## Verified builds

The clean CPU-only build used CMake and Ninja with LLVM 23.1.1. Four fixture sets passed exact, fast, recording and AOT runs. The same suite passed from a clean Git archive with no game or dependency directories. Both original minimal variants contain 1,664 input bytes. The Wasm emitter built 16 modules, with two indirect-entry fallbacks, and the supported variants matched the reference. Thirteen executions of the listed native and generator checks passed, including VIF1/UNPACK, deferral on/off and VU0/VU1 regression suites. The audit CMake link exports the Windows synchronization library to clients even when no GPU backend is linked.

Separate fresh Release builds regenerated EE and VU programs from the local PAL disc and optional raw recordings. The 16:9 helper recreated the expected patched ELF and runtime patch list. Its patched ELF hash also matched the historical local build. The CMake caches selected this repository as the source directory and left the prebuilt-runtime option empty. 4:3 registered 333 compiled VU1 programs and 16:9 registered 315.

Both runners reproduced 62 frames from copied checkpoints with movement. Eight comparable frames matched the Stage 4 captures byte for byte, four in each profile. The fresh 4:3 runner also produced three 4× captures at 2560 × 1792 using D3D12/DXC and completed its 16-frame run.

A separate 4:3 built from the PAL disc alone reached the title, new-game selection and opening cinematic in a 7,900-frame cold run, with EE live filling missing entries locally. Longer gameplay validation used the copied checkpoints. Transitions between playable levels still need a dedicated test.

## Checklist for a later private push

Run the audit from the prepared checkout before publication. It lists and hashes every tracked file and scans reachable historical blobs for binary payloads, excluded paths, known secret signatures and local path fragments. Review changes and the manifest when introducing new assets or dependencies. The inventory and the report about the local preserved installations stay in the ignored `.audit/` directory.

```powershell
git status --short
python scripts/audit_publication.py
git fsck --full
git remote -v
git tag --list
```

When you decide to publish, create an empty **private** repository in the intended account. Use the exact branch and the annotated candidate tag. The following is the GitHub CLI route. Use GitHub's browser authentication when prompted.

```powershell
gh auth login
gh repo create PS2Native-Maximo-Recomp --private --source=. --remote=origin
gh repo view --json nameWithOwner,visibility
git push --atomic --set-upstream origin main refs/tags/maximo-rex-stage4-candidate-1
```

If you create the empty private repository in the GitHub website, use `git remote add origin https://github.com/<OWNER>/PS2Native-Maximo-Recomp.git` before the push instead of `gh repo create`. [GitHub CLI create command](https://cli.github.com/manual/gh_repo_create) · [GitHub's guide to adding local code](https://docs.github.com/en/repositories/creating-and-managing-repositories/adding-locally-hosted-code-to-github)
