# ── aw::tobus ────────────────────────────────────────────────────────────
# Everyday build targets.
#
#   make build | test | clean   (BUILD_TYPE=Debug|Release, default Debug)
#
#   make release [PUSH=1]        cut a SemVer release from CHANGELOG.md
#   make release-dry-run         preview the next version without changing anything
#   make changelog [FORCE=1]     update [Unreleased] from git log (conventional commits)
#   make changelog-dry-run       preview changelog update
#
# Examples:
#   make build BUILD_TYPE=Release
#   make test BUILD_TYPE=Release

BUILD_TYPE ?= Debug
PRESET     := $(if $(filter Release,$(BUILD_TYPE)),release,debug)

.PHONY: build test clean lint format release release-dry-run changelog changelog-dry-run

build:
	cmake --preset $(PRESET)
	cmake --build --preset $(PRESET)

test: build
	@if [ -f tests/CMakeLists.txt ]; then ctest --preset $(PRESET); \
	else echo "No tests/ directory."; fi

lint: build
	@if command -v clang-tidy >/dev/null 2>&1; then \
	    find include src -name '*.hpp' -o -name '*.cpp' | xargs clang-tidy -p build/$(PRESET); \
	else echo "clang-tidy not found"; fi

format:
	@if command -v clang-format >/dev/null 2>&1; then \
	    find include src examples tests -name '*.hpp' -o -name '*.cpp' 2>/dev/null | xargs clang-format -i; \
	else echo "clang-format not found (>= 15 required)"; fi

clean:
	rm -rf build

# Detect POSIX shell: MSYS2/MinGW/Cygwin/WSL all have /usr/bin.
# Only pure Windows (no /usr/bin) falls back to PowerShell.
IS_POSIX := $(strip $(if $(wildcard /usr/bin),1,))

release:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release.ps1 $(if $(PUSH),-Push)
else
	bash scripts/release.sh $(if $(PUSH),--push)
endif

release-dry-run:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/release.ps1 -DryRun
else
	bash scripts/release.sh --dry-run
endif

changelog:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/changelog-update.ps1 $(if $(FORCE),-Force)
else
	bash scripts/changelog-update.sh $(if $(FORCE),--force)
endif

changelog-dry-run:
ifneq ($(IS_POSIX),1)
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/changelog-update.ps1 -DryRun
else
	bash scripts/changelog-update.sh --dry-run
endif
