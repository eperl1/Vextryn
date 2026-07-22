cmake_minimum_required(VERSION 3.10)
project(Vextryn_Air VERSION 0.1.0 LANGUAGES C CXX ASM)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Compiler flags for kernel development
set(COMMON_FLAGS "-ffreestanding -fno-builtin -nostdlib -fno-pic -fno-pie -fno-stack-protector -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mgeneral-regs-only")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -include stdint.h -include stddef.h -include stdbool.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++20 -fno-exceptions -fno-rtti -include stdint.h -include stddef.h -include stdbool.h")

# Enable NASM
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_FLAGS "-f elf64")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/userspace/libc/include)

file(GLOB_RECURSE KERNEL_SOURCES
    "boot/*.c" "boot/*.S" "boot/*.asm"
    "kernel/*.c" "kernel/*.S" "kernel/*.asm"
    "drivers/*.c" "drivers/*.S" "drivers/*.asm"
    "net/*.c" "net/*.S" "net/*.asm"
    "fs/*.c" "fs/*.S" "fs/*.asm"
)
list(APPEND KERNEL_SOURCES "gui/compositor/vxair_vxcomp.cpp")
list(FILTER KERNEL_SOURCES EXCLUDE REGEX "uefi_bootloader\\.c$")

add_executable(vextryn_air.elf ${KERNEL_SOURCES})
target_link_options(vextryn_air.elf PRIVATE -T ${CMAKE_SOURCE_DIR}/build/linker.ld -nostdlib -no-pie)

file(GLOB_RECURSE LIBC_SOURCES "userspace/libc/src/*.c")
add_library(vxair_libc STATIC ${LIBC_SOURCES})

add_executable(init.elf "userspace/init/src/vxair_init.c" "userspace/crt/src/vxair_crt0.c")
target_link_libraries(init.elf vxair_libc)
target_link_options(init.elf PRIVATE -nostdlib -no-pie)

add_executable(shell.elf "userspace/shell/src/vxair_shell.c" "userspace/crt/src/vxair_crt0.c")
target_link_libraries(shell.elf vxair_libc)
target_link_options(shell.elf PRIVATE -nostdlib -no-pie)
target_include_directories(shell.elf PRIVATE ${CMAKE_SOURCE_DIR}/userspace/shell/include)
