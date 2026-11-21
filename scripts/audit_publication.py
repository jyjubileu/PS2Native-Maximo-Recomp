#!/usr/bin/env python3
"""Audit the source at HEAD and all Git objects that a branch/tag push can reach."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOT_FILES = {".gitattributes", ".gitignore", "CMakeLists.txt", "LICENSE", "README.md", "Start.cmd"}
SOURCE_DIRS = {".github", "app", "cmake", "host", "recomp", "runtime", "scripts", "tests", "web"}
ALLOWED_SUFFIXES = {
    ".awk", ".c", ".cc", ".cmake", ".cmd", ".cpp", ".cs", ".css", ".csv", ".diff",
    ".h", ".hpp", ".html", ".inc", ".js", ".json", ".md", ".mjs", ".ps1", ".py",
    ".sh", ".txt", ".wgsl", ".yaml", ".yml",
}
NOTICE_NAMES = {"CMakeLists.txt", "LICENSE", "NOTICE", "COPYING"}
FORBIDDEN_PARTS = {"build", "games", "generated", "isos", "third_party", ".audit", ".local",
                   ".git", "__pycache__", "overlays", "vu1-live", "ee-live", "sstates"}
GAME_FILENAME = re.compile(
    r"(?i)(?:system\.cnf|(?:sles|slus|scus|sces|slpm|slps)[_-]\d+[._]\d+|"
    r"unit_\d+\.(?:cpp|hpp)|m_[0-9a-f]{8}_[0-9a-f]{8}_unit_\d+\.cpp|"
    r"vu[01]_(?:[0-9a-f]{16}|\d{3}|table)\.cpp)$"
)
CREDENTIAL_PATTERNS = (
    ("private-key", r"-----BEGIN [A-Z ]*PRIVATE KEY-----"),
    ("github-token", r"\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{36,})\b"),
    ("aws-key-id", r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b"),
    ("cloud-api-key", r"\b(?:AIza[A-Za-z0-9_-]{35}|sk-(?:proj-)?[A-Za-z0-9_-]{32,})\b"),
    ("slack-token", r"\bxox[baprs]-[A-Za-z0-9-]{20,}\b"),
    ("credential-in-url", r"https?://[^ \t\r\n/@:]+:[^ \t\r\n/@]+@"),
    ("jwt-like", r"\beyJ[A-Za-z0-9_-]{16,}\.eyJ[A-Za-z0-9_-]{16,}\.[A-Za-z0-9_-]{12,}\b"),
)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def tree(commit):
    for item in git("ls-tree", "-r", "-z", commit).split(b"\0"):
        if item:
            header, filename = item.decode("utf-8").split("\t", 1)
            mode, kind, oid = header.split()
            yield filename, (mode, kind, oid)


def category(name):
    path = PurePosixPath(name)
    first = path.parts[0]
    if name in ROOT_FILES:
        return "license" if name == "LICENSE" else "project"
    if first == "docs":
        return "documentation or audit patch"
    if first == "tools":
        return "upstream MIT/BSD toolkit"
    if first == "tests":
        return "test source or synthetic oracle"
    if path.name in NOTICE_NAMES:
        return "license notice"
    return "source or build configuration"


def audit_path(name, mode, kind):
    path = PurePosixPath(name)
    first = path.parts[0]
    if (path.is_absolute() or ".." in path.parts or "\\" in name or
            any(piece.casefold() in FORBIDDEN_PARTS for piece in path.parts)):
        return "excluded path"
    if mode not in ("100644", "100755") or kind != "blob":
        return "non-regular Git object"
    if GAME_FILENAME.fullmatch(path.name):
        return "game input or generated source filename"
    in_tree = (first in SOURCE_DIRS or first == "docs" or name.startswith("tools/ps2_disc_toolkit/"))
    if name not in ROOT_FILES and (not in_tree or
            (path.suffix not in ALLOWED_SUFFIXES and path.name not in NOTICE_NAMES)):
        return "outside reviewed source allowlist"
    return ""


def audit_blob(blob, name, checks):
    if len(blob) > 2 * 1024 * 1024:
        return "unreviewed large source blob"
    # The unchanged upstream regex in speedcheck.py contains one literal backspace.
    controls = [byte for byte in blob if byte < 32 and byte not in (9, 10, 13)]
    upstream_regex = name == "scripts/speedcheck.py" and controls == [8] and hashlib.sha256(blob).hexdigest() == "78ac3deb4e34797fbc7e03c3bc6e88bc2a4f2e7d6a9535a365aca0a06d3c886a"
    if controls and not upstream_regex:
        return "binary or non-text payload"
    if blob.startswith((b"MZ", b"\x7fELF", b"PK\x03\x04", b"\x1f\x8b")):
        return "executable/archive signature"
    try:
        text = blob.decode("utf-8-sig")
    except UnicodeError:
        return "non-UTF-8 blob"
    for label, pattern in checks:
        match = pattern.search(text)
        if match:
            return f"{label} at line {text.count(chr(10), 0, match.start()) + 1}"
    return ""


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=ROOT / ".audit")
    ap.add_argument("--deny-fragment", action="append", default=[],
                    help="Additional private fragment to reject without printing the match")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    checks = [(k, re.compile(v)) for k, v in CREDENTIAL_PATTERNS]
    user_home = str(Path.home())
    private_roots = [user_home, user_home.replace("\\", "/"), *args.deny_fragment]
    checks.extend(("local/private path", re.compile(re.escape(part), re.IGNORECASE))
                  for part in set(private_roots) if len(part) > 4)
    current = dict(tree("HEAD"))
    commits = git("rev-list", "--all").decode().splitlines()
    history = {}
    for commit in commits:
        for name, attrs in tree(commit):
            history.setdefault((name, attrs), commit)
    known = {}
    errors = []
    for (name, attrs), commit in history.items():
        mode, kind, oid = attrs
        issue = audit_path(name, mode, kind)
        if issue:
            errors.append((commit[:12], name, issue))
            continue
        if oid not in known:
            blob = git("cat-file", "blob", oid)
            known[oid] = (len(blob), hashlib.sha256(blob).hexdigest(), audit_blob(blob, name, checks))
        if known[oid][2]:
            errors.append((commit[:12], name, known[oid][2]))
    out = args.out / "tracked-files-sha256.csv"
    if not errors:
        with out.open("w", encoding="utf-8", newline="") as f:
            wr = csv.writer(f)
            wr.writerow(("path", "bytes_in_git", "sha256_in_git", "git_blob", "category"))
            for path, (_, _, oid) in current.items():
                size, checksum, _ = known[oid]
                wr.writerow((path, size, checksum, oid, category(path)))
    result = {
        "head": git("rev-parse", "HEAD").decode().strip(),
        "tracked_files": len(current),
        "reachable_commits": len(commits),
        "audited_path_blob_pairs": len(history),
        "unique_text_blobs": len(known),
        "tracked_bytes": sum(known[oid][0] for _, _, oid in current.values() if oid in known),
        "issues": errors,
    }
    (args.out / "git-audit.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
    raise SystemExit(bool(errors))


if __name__ == "__main__":
    main()
