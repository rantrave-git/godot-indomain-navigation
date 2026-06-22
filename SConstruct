#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
def make_macro(prefix, t: str):
    if not t: return None
    return f"{prefix}_{t.upper()}"

opts = Variables('custom.py', ARGUMENTS)
opts.Add(BoolVariable("release", "Set defaults to build for use in production", True))
opts.Add(BoolVariable("no_tools", "Disable editor tools", False))
opts.Add(BoolVariable("no_sequential_iteration", "Use associative collection for the agents' iteration", False))
opts.Add(("trace", "trace log modules"))
opts.Add(("debug", "debug log modules"))
opts.Update(env)

trace = env.get('trace', '')
if trace:
    trace_defines = ["INDOMAIN_DEBUG_TRACE"] + [y for y in (make_macro("INDOMAIN_TRACE", x) for x in trace.split(',')) if y]
    print(f"Traces: {debug_defines[1:]}")
else:
    trace_defines = []

debug = env.get('debug', '')
if debug:
    debug_defines = [y for y in (make_macro("INDOMAIN_DEBUG", x) for x in debug.split(',')) if y]
    print(f"Debug defines: {debug_defines[1:]}")
else:
    debug_defines = []

release = env.get('release', '')
if release:
    print(f"Release build")
    release_flags = ["-O2", "-mmmx", "-msse4.1", "-ffast-math", "-fassociative-math"]
else:
    release_flags = []
    debug_defines.append("NDEBUG")

no_tools = env.get('no_tools', '')
if no_tools:
    tools_defines = []
else:
    tools_defines = ["TOOLS_ENABLED"]

no_seq = env.get('no_sequential_iteration', '')
if no_seq:
    no_seq_defines = []
else:
    no_seq_defines = ["INDOMAIN_SEQUENTIAL_ITERATION"]

env.Append(
    CPPPATH=["src/"],
    CXXFLAGS=release_flags,
    CPPDEFINES=tools_defines + no_seq_defines + trace_defines + debug_defines)
sources = Glob("src/*.cpp") + Glob('src/classes/*.cpp')

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libin-domain-movement.{}.{}.framework/libin-domain-movement.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/libin-domain-movement.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/libin-domain-movement.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/bin/libin-domain-movement{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
