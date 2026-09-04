BUILD_DIR := build

# Ninja's no-op build is much quicker, but fall back to CMake's default so the
# wrapper still works without it.
GENERATOR := $(shell command -v ninja >/dev/null 2>&1 && echo "-G Ninja")

.DEFAULT_GOAL := build

.PHONY: configure build test run clean

configure: $(BUILD_DIR)/CMakeCache.txt

$(BUILD_DIR)/CMakeCache.txt:
	cmake -S . -B $(BUILD_DIR) $(GENERATOR)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	@./$(BUILD_DIR)/stepseq

clean:
	rm -rf $(BUILD_DIR)
