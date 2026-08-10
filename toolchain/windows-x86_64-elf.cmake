set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(DEFINED ENV{VEXTRYN_ELF_TOOLCHAIN} AND NOT "$ENV{VEXTRYN_ELF_TOOLCHAIN}" STREQUAL "")
    file(TO_CMAKE_PATH "$ENV{VEXTRYN_ELF_TOOLCHAIN}" TOOLCHAIN_ROOT)
else()
    file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/Vextryn-Air/toolchains/x86_64-elf-tools" TOOLCHAIN_ROOT)
endif()

set(TOOLCHAIN_BIN "${TOOLCHAIN_ROOT}/bin")

set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN}/x86_64-elf-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN}/x86_64-elf-g++.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN}/x86_64-elf-gcc.exe")
set(CMAKE_AR "${TOOLCHAIN_BIN}/x86_64-elf-ar.exe")
set(CMAKE_RANLIB "${TOOLCHAIN_BIN}/x86_64-elf-ranlib.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN_BIN}/x86_64-elf-objcopy.exe")
set(CMAKE_NM "${TOOLCHAIN_BIN}/x86_64-elf-nm.exe")
set(CMAKE_STRIP "${TOOLCHAIN_BIN}/x86_64-elf-strip.exe")
set(CMAKE_LINKER "${TOOLCHAIN_BIN}/x86_64-elf-ld.exe")

find_program(NASM_EXE
    NAMES nasm.exe
    PATHS
        "C:/Program Files/NASM"
        "C:/Program Files (x86)/NASM"
        "C:/Users/eperlinovitch/AppData/Local/Microsoft/WinGet/Links"
    NO_DEFAULT_PATH
)
if(NOT NASM_EXE)
    find_program(NASM_EXE NAMES nasm.exe)
endif()
if(NOT NASM_EXE)
    message(FATAL_ERROR "NASM was not found. Install it with winget install --source winget NASM.NASM")
endif()
set(CMAKE_ASM_NASM_COMPILER "${NASM_EXE}")
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
