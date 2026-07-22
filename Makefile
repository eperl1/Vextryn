# Top-level Makefile for Vextryn Air OS

.PHONY: all clean run iso menuconfig

all:
	@echo "=> Building Vextryn Air OS..."
	@mkdir -p build/out
	@cd build && cmake .. && make -j$$(nproc)
	@echo "=> Build complete."

clean:
	@echo "=> Cleaning build artifacts..."
	@rm -rf build/out
	@cd build && make clean || true

run: all
	@echo "=> Launching Vextryn Air in QEMU..."
	@./scripts/run_qemu.sh

iso: all
	@echo "=> Generating Bootable ISO..."
	@./scripts/generate_iso.sh
	@echo "=> ISO generated in build/out/vextryn-air.iso"
