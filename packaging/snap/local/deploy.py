#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Collect runtime ELF dependencies and license notices, without a deployer binary."""
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

root, qt, graph = map(Path, sys.argv[1:])
# The base supplies glibc; the gpu-2604 part handles graphics cleanup at prime.
external = re.compile(
    r"^(ld-linux.*|lib(c|m|pthread|dl|rt|resolv|util|anl)\.so\..*)$"
)


def elf(path):
    return path.is_file() and path.open("rb").read(4) == b"\x7fELF"


queue = [p for p in root.rglob("*") if not p.is_symlink() and elf(p)]
seen = set()
while queue:
    obj = queue.pop()
    if obj in seen:
        continue
    seen.add(obj)
    result = subprocess.run(["ldd", str(obj)], text=True, capture_output=True)
    if "not found" in result.stdout:
        sys.exit(f"Unresolved build-time ELF dependency of {obj}:\n{result.stdout}")
    for soname, filename in re.findall(r"^\s*(\S+) => (/\S+)", result.stdout, re.M):
        if external.fullmatch(soname):
            continue
        source = Path(filename).resolve()
        # Debian dependencies already staged retain their package layout/notices.
        staged = [p for p in root.rglob(soname) if p.is_file()]
        if staged:
            continue
        # Keep distro paths so gpu-snap's architecture-specific lists match.
        # Qt and Conan libraries stay together in the private runtime directory.
        if source.is_relative_to("/usr/lib") or source.is_relative_to("/lib"):
            target = root / source.parent.relative_to("/") / soname
        else:
            target = root / "usr/lib" / soname
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        queue.append(target)

licenses = root / "usr/share/doc/plotjuggler/third-party"
licenses.mkdir(parents=True, exist_ok=True)
if (qt / "licenses").is_dir():
    shutil.copytree(qt / "licenses", licenses / "qt", dirs_exist_ok=True)
data = json.loads(graph.read_text())
for node in data["graph"]["nodes"].values():
    folder = node.get("package_folder")
    if folder and (Path(folder) / "licenses").is_dir():
        name = node["ref"].split("#")[0].replace("/", "-")
        shutil.copytree(Path(folder) / "licenses", licenses / name, dirs_exist_ok=True)
