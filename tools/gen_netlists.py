#!/usr/bin/env python3
"""Writes <out>/<m>/netlists.h from vendor/lemus/<m>/core/netlists/*.cir (what Lemus's tools/cmake/netlists.cmake does) and the
include shim <out>/<m>/core -> vendor/lemus/<m>/core, so the vendored core compiles unchanged:  gen_netlists.py tr909 909MPC/build/inc"""
import glob, os, sys
m, out = sys.argv[1], sys.argv[2]
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = os.path.join(root, "vendor/lemus", m, "core")
names = sorted(os.path.splitext(os.path.basename(f))[0] for f in glob.glob(os.path.join(src, "netlists/*.cir")))
t = "// Generated from vendor/lemus/%s/core/netlists/*.cir by tools/gen_netlists.py.\n#pragma once\n\n#include <cstring>\n\nnamespace %s::netlists {\n\n" % (m, m)
for n in names:
    body = open(os.path.join(src, "netlists", n + ".cir")).read()
    assert ')cir"' not in body
    t += 'inline constexpr const char *%s = R"cir(%s)cir";\n\n' % (n, body)
t += "inline const char *find(const char *name)\n{\n\tstruct Entry {\n\t\tconst char *name, *text;\n\t};\n\tstatic const Entry all[] = {\n"
for n in names: t += '\t\t{"%s", %s},\n' % (n, n)
t += "\t};\n\tfor (const auto &e : all)\n\t\tif (std::strcmp(e.name, name) == 0)\n\t\t\treturn e.text;\n\treturn nullptr;\n}\n\n} // namespace %s::netlists\n" % m
d = os.path.join(root, out, m); os.makedirs(d, exist_ok=True)
open(os.path.join(d, "netlists.h"), "w").write(t)
link = os.path.join(d, "core")
if os.path.lexists(link): os.remove(link)
os.symlink(src, link)
print(m, len(names), "netlists")
