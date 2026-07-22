# Vextryn Air Contributor Guide

Welcome to the Vextryn Air project! As an OS designed for minimal footprint and maximum capability, we enforce strict coding guidelines.

## Prime Directives
1. **No Placeholders**: Every submitted file must be fully implemented. No empty bodies or `TODO` stubs are permitted in the main branch.
2. **Zero External Dependencies**: The kernel and drivers must not rely on external libraries.
3. **Code Quality**: All code must compile cleanly with `-Wall -Wextra -Werror`.

## Language Standards
- **Kernel & Drivers**: C17 (`-std=c17 -ffreestanding`)
- **User Space & GUI**: C++20 (`-std=c++20 -fno-exceptions -fno-rtti` where applicable)

## Naming Conventions
- **C Code**: All global symbols, functions, and structs must be prefixed with `vxair_` (e.g., `vxair_pmm_alloc_page`).
- **C++ Code**: All classes and functions must reside within the `Vx::` namespace.

## Documentation
- Every public API function must include a Doxygen-style comment block explaining its purpose, parameters, and return value.

```c
/**
 * @brief Allocates a contiguous block of physical pages.
 * 
 * @param count Number of 4K pages to allocate.
 * @return Physical address of the allocated block, or 0 on failure.
 */
uint64_t vxair_pmm_alloc_pages(size_t count);
```
