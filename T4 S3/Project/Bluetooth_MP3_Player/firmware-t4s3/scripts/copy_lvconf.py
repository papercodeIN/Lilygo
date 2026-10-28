"""PlatformIO pre-build hook.

Places include/lv_conf.h where LVGL's internal header always finds it:
lvgl resolves `#include "../../lv_conf.h"` relative to
.pio/libdeps/<env>/lvgl/src/, i.e. lv_conf.h must sit in .pio/libdeps/<env>/.
This guarantees the *library's* translation units (font selections, color
swap, allocator) compile with the same config as the project's own code,
independent of PlatformIO include-path ordering.
"""
import os
import shutil

Import("env")

include_conf = os.path.join(env["PROJECT_INCLUDE_DIR"], "lv_conf.h")
libdeps = env["PROJECT_LIBDEPS_DIR"]
target_dir = os.path.join(libdeps, env["PIOENV"])

if os.path.isfile(include_conf):
    os.makedirs(target_dir, exist_ok=True)
    dst = os.path.join(target_dir, "lv_conf.h")
    if not os.path.isfile(dst) or os.path.getmtime(dst) < os.path.getmtime(include_conf):
        shutil.copyfile(include_conf, dst)
        print("copy_lvconf: %s -> %s" % (include_conf, dst))
else:
    print("copy_lvconf: WARNING - %s not found" % include_conf)
