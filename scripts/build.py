#!/usr/bin/env python3
import os, subprocess, sys

def run(cmd_list):
    print(f"Running: {' '.join(cmd_list)}")
    # Removing shell=True forces the process execution to obey cwd
    if subprocess.call(cmd_list, cwd=os.getcwd()) != 0:
        print("Build failed!")
        sys.exit(1)

def main():
    os.chdir(os.path.join(os.path.dirname(__file__), '..'))
    
    run(["mkdir", "-p", "build-out"])
    os.chdir("build-out")
    
    run(["cmake", "..", "-DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64-elf.cmake"])
    run(["cmake", "--build", ".", "--parallel"])
    
    os.chdir("..")
    run(["mkdir", "-p", "iso_root/vextryn"])
    run(["cp", "build-out/bin/vextryn_air.elf", "iso_root/vextryn/kernel.elf"])
    run(["grub-mkrescue", "-o", "vextryn-air.iso", "iso_root/"])
    print("Build complete: vextryn-air.iso")

if __name__ == "__main__":
    main()
