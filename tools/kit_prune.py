#!/usr/bin/env python3
"""Prune the self-compile kit stage and rebuild its zip.

The framework's kit packager stages the whole psxrecomp/ and recomp-ui/ trees.
Players don't need the developer's local build leftovers, nor recomp-ui's
art for other consoles (which includes third-party box art).
This removes them from the stage and re-zips it, so the IP audit runs on
exactly what ships.

Usage: python tools/kit_prune.py <stage_dir> <zip_path>
"""
import os, shutil, sys, zipfile

PRUNE_DIRS = [
    # Only local build leftovers. Everything else in psxrecomp/ stays: the
    # player's first-run build configures with tests on and references tests,
    # docs and test ROMs (removing runtime/tests broke the emitter configure).
    "psxrecomp/.cache",                  # local download/extract cache (recreated on demand)
    "psxrecomp/recompiler/build",        # developer's emitter build (absolute local paths)
]
KEEP_CONSOLE = "psx"                     # recomp-ui/assets/consoles/<console>: only PlayStation is used
# recomp-ui's PlayStation profile shows the PlayStation logo (a Sony trademark).
# The player's build requires a file of that name, so it is replaced with the
# project's own emblem rather than deleted. The launcher shows the emblem anyway
# (LAUNCHER_BRAND in CMakeLists.txt); this keeps the logo out of the zip.
TRADEMARK_FILES = ["brand_psx.tga"]
OWN_EMBLEM = "assets/launcher_brand.tga"


def main():
    stage, out = sys.argv[1], sys.argv[2]
    removed = 0
    for rel in PRUNE_DIRS:
        p = os.path.join(stage, *rel.split("/"))
        if os.path.isdir(p):
            removed += sum(len(f) for _, _, f in os.walk(p))
            shutil.rmtree(p)
            print("pruned", rel)
    consoles = os.path.join(stage, "recomp-ui", "assets", "consoles")
    if os.path.isdir(consoles):
        for name in sorted(os.listdir(consoles)):
            if name != KEEP_CONSOLE:
                p = os.path.join(consoles, name)
                removed += sum(len(f) for _, _, f in os.walk(p))
                shutil.rmtree(p)
                print("pruned recomp-ui/assets/consoles/" + name)
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", OWN_EMBLEM), "rb") as f:
        emblem = f.read()
    for dp, _dn, fn in os.walk(stage):
        for f in fn:
            if f.lower() in TRADEMARK_FILES:
                with open(os.path.join(dp, f), "wb") as out_f:
                    out_f.write(emblem)
                print("replaced", os.path.relpath(os.path.join(dp, f), stage).replace("\\", "/"), "with", OWN_EMBLEM)
    tmp = out + ".tmp"
    with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as z:
        for dp, _dn, fn in os.walk(stage):
            for f in fn:
                full = os.path.join(dp, f)
                z.write(full, os.path.relpath(full, stage).replace("\\", "/"))
    os.replace(tmp, out)
    print("removed %d files; rebuilt %s (%.1f MB)" % (removed, out, os.path.getsize(out) / 1e6))


if __name__ == "__main__":
    main()
