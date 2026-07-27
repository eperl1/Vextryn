#!/usr/bin/env python3
import os, subprocess, sys
import shsplit # standard library equivalent, or just split strings manually

def run(cmd_list):
    print(f"Running: {' '.join(cmd_list)}")
    # Removing shell=True and using a list forces Python to respect cwd
    if subprocess.call(cmd_list, cwd=os.getcwd()) != 0:
        print("Build failed!")
        sys.exit(1)

def main():
    os.chdir(os.path.join(os.path.dirname(__file__), '..'))
    
    # Pass arguments as lists instead of single strings
    run(["mkdir", "-p", "build-out"])
    os.chdir("build-out")
    
    run(["cmake", "..", "-DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64-vxair-elf.cmake"])
    run(["cmake", "--build", ".", "--parallel"])
    
    os.chdir("..")
    run(["mkdir", "-p", "iso_root/vextryn"]) # Added safety check for folder existence
    run(["cp", "build-out/bin/vextryn_air.elf", "iso_root/vextryn/kernel.elf"])
    run(["grub-mkrescue", "-o", "vextryn-air.iso", "iso_root/"])
    print("Build complete: vextryn-air.iso")

if __name__ == "__main__":
    main()
