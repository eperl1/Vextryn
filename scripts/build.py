#!/usr/bin/env python3
import os, subprocess, sys
from pathlib import Path
import shutil

def find_tool(candidates, name):
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return str(Path(candidate))
    resolved = shutil.which(name)
    if resolved:
        return resolved
    return None

def run(cmd_list):
    print(f"Running: {' '.join(cmd_list)}")
    # Removing shell=True forces the process execution to obey cwd
    if subprocess.call(cmd_list, cwd=os.getcwd()) != 0:
        print("Build failed!")
        sys.exit(1)


def write_windows_headers(header_dir: Path):
    header_dir.mkdir(parents=True, exist_ok=True)

    string_h = header_dir / "string.h"
    if not string_h.exists():
        string_h.write_text(
            """#ifndef VEXTRYN_WINDOWS_STRING_H
#define VEXTRYN_WINDOWS_STRING_H

#include <stddef.h>

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

#endif
""",
            encoding="utf-8",
        )

    stdlib_h = header_dir / "stdlib.h"
    if not stdlib_h.exists():
        stdlib_h.write_text(
            """#ifndef VEXTRYN_WINDOWS_STDLIB_H
#define VEXTRYN_WINDOWS_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);

#endif
""",
            encoding="utf-8",
        )

    sys_dir = header_dir / "sys"
    sys_dir.mkdir(parents=True, exist_ok=True)
    sys_types_h = sys_dir / "types.h"
    if not sys_types_h.exists():
        sys_types_h.write_text(
            """#ifndef VEXTRYN_WINDOWS_SYS_TYPES_H
#define VEXTRYN_WINDOWS_SYS_TYPES_H

#include <stddef.h>

typedef long ssize_t;
typedef long off_t;
typedef long pid_t;

#endif
""",
            encoding="utf-8",
        )

def main():
    root = Path(__file__).resolve().parent.parent
    os.chdir(root)

    build_dir = root / ("build-out-windows-fresh" if os.name == "nt" else "build-out")
    build_dir.mkdir(parents=True, exist_ok=True)
    os.chdir(build_dir)
    toolchain = "../toolchain/x86_64-elf.cmake"
    if os.name == "nt":
        toolchain = "../toolchain/windows-x86_64-elf.cmake"
        default_toolchain = Path(os.environ.get("LOCALAPPDATA", "")) / "Vextryn-Air" / "toolchains" / "x86_64-elf-tools"
        toolchain_root = Path(os.environ.get("VEXTRYN_ELF_TOOLCHAIN", str(default_toolchain)))
        if not toolchain_root.exists():
            print("Build failed: Windows x86_64-elf toolchain not found.")
            print("Expected it at:", toolchain_root)
            print("Run scripts/windows/bootstrap_toolchain.ps1 first.")
            sys.exit(1)
        os.environ["VEXTRYN_ELF_TOOLCHAIN"] = str(toolchain_root)
        header_dir = build_dir / "windows-headers"
        write_windows_headers(header_dir)
        existing_cpath = os.environ.get("CPATH", "")
        cpath_parts = [str(header_dir)]
        if existing_cpath:
            cpath_parts.append(existing_cpath)
        os.environ["CPATH"] = os.pathsep.join(cpath_parts)
        existing_cxxpath = os.environ.get("CPLUS_INCLUDE_PATH", "")
        cxx_parts = [str(header_dir)]
        if existing_cxxpath:
            cxx_parts.append(existing_cxxpath)
        os.environ["CPLUS_INCLUDE_PATH"] = os.pathsep.join(cxx_parts)

    cmake = find_tool(
        [
            r"C:\Program Files\CMake\bin\cmake.exe",
            r"C:\msys64\mingw64\bin\cmake.exe",
            r"C:\Users\eperlinovitch\AppData\Local\Microsoft\WinGet\Links\cmake.exe",
        ],
        "cmake",
    )
    ninja = find_tool(
        [
            r"C:\Users\eperlinovitch\AppData\Local\Microsoft\WinGet\Links\ninja.exe",
            r"C:\msys64\mingw64\bin\ninja.exe",
            r"C:\Program Files\Ninja\ninja.exe",
        ],
        "ninja",
    )
    if not cmake:
        print("Build failed: cmake not found.")
        sys.exit(1)
    if not ninja:
        print("Build failed: ninja not found.")
        sys.exit(1)

    run([cmake, "..", "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={ninja}", f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"])
    run([cmake, "--build", ".", "--parallel"])

    os.chdir("..")
    iso_root = root / "iso_root" / "vextryn"
    iso_root.mkdir(parents=True, exist_ok=True)
    kernel_source = root / ("build-out-windows-fresh" if os.name == "nt" else "build-out") / "bin" / "vextryn_air.elf"
    if not kernel_source.exists():
        kernel_source = root / "build" / "bin" / "vextryn_air.elf"
    shutil.copy2(kernel_source, iso_root / "kernel.elf")
    grub = find_tool(
        [
            r"C:\msys64\usr\bin\grub-mkrescue.exe",
            r"C:\msys64\mingw64\bin\grub-mkrescue.exe",
        ],
        "grub-mkrescue",
    )
    if grub:
        run([grub, "-o", "vextryn-air.iso", "iso_root/"])
        print("Build complete: vextryn-air.iso")
    else:
        print("Kernel build complete, but grub-mkrescue was not found on this Windows setup.")
        print("Boot the kernel directly with scripts/windows/launch_kernel.ps1")

if __name__ == "__main__":
    main()
