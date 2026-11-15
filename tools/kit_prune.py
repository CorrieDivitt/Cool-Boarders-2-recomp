#!/usr/bin/env python3
"""Prune the self-compile kit stage and rebuild its zip.

The framework's kit packager stages the whole psxrecomp/ and recomp-ui/ trees.
Players need neither the framework's tests, test ROMs, docs and local caches,
nor recomp-ui's art for other consoles (which includes third-party box art).
This removes them from the stage and re-zips it, so the IP audit runs on
exactly what ships.

Usage: python tools/kit_prune.py <stage_dir> <zip_path>
"""
import os, shutil, sys, zipfile

PRUNE_DIRS = [
    "psxrecomp/runtime/tests",          # framework test fixtures (synthetic memcards etc.)
    "psxrecomp/tools/cycle_testrom",     # framework CPU timing test ROM
    "psxrecomp/docs",                    # developer documentation
    "psxrecomp/.cache",                  # local download/extract cache
    "psxrecomp/recompiler/build",        # framework developer build (emitters ship separately)
]
KEEP_CONSOLE = "psx"                     # recomp-ui/assets/consoles/<console>: only PlayStation is used


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
