MAKEFLAGS += --no-print-directory

BUILD_DIR = build
SOURCE_DIRS = source  # only used for clang-format
SOURCE_FILES = $(shell find $(SOURCE_DIRS) -type f -iregex ".*\.\(c\|h\)\(pp\|xx\|\)")
EXTERNAL_DIR = external
CMAKE_FLAGS =

.PHONY: build
build: prepare-build shaders assets/building.obj
	$(MAKE) -C $(BUILD_DIR)

.PHONY: build-tests
build-tests: prepare-build
	$(MAKE) -C $(BUILD_DIR) tests

.PHONY: shaders
shaders: prepare-build
	$(MAKE) -C $(BUILD_DIR) shaders

.PHONY: download-external
download-external:
	$(MAKE) -C $(EXTERNAL_DIR)

compile_commands.json:
	ln -fs build/compile_commands.json .

.PHONY: prepare-build
prepare-build: download-external compile_commands.json
	[ -d "$(BUILD_DIR)" ] || mkdir $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) ..

.PHONY: debug
debug: CMAKE_FLAGS += -DCMAKE_BUILD_TYPE=Debug
debug: build;


.PHONY: run-tests
run-tests:
	$(BUILD_DIR)/source/tests

.PHONY: test
test: build-tests run-tests

.PHONY: run
run: build
	cd $(BUILD_DIR)/source && ./main

.PHONY: lint
lint:
	@clang-format --version | grep -qE "[1-9][0-9]+\.[0-9]+\.[0-9]+" || \
		(echo "Clang 10 or later is required for linting." && exit 1)
	clang-format -n --Werror $(SOURCE_FILES)

.PHONY: codeformat
codeformat:
	clang-format -i $(SOURCE_FILES)


.PHONY: clean
clean:
	rm -fr $(BUILD_DIR)/
	find . -name "CMakeCache.txt" -exec rm {} \;
	rm -fr $(EXTERNAL_DIR)/VulkanMemoryAllocator/build
	rm -fr assets/building.obj

assets/building.obj:
	cd assets && gunzip -k building.obj.gz
