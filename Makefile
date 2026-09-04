BUILD_DIR := build

.DEFAULT_GOAL := build

.PHONY: configure build test run clean

configure: $(BUILD_DIR)/CMakeCache.txt

$(BUILD_DIR)/CMakeCache.txt:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	@./$(BUILD_DIR)/stepseq

clean:
	rm -rf $(BUILD_DIR)
