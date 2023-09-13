GLM_VERSION = 0.9.9.8

.PHONY: all
all: vma

.PHONY: vma
vma:
	cd ./VulkanMemoryAllocator && \
		cmake -S . -B build && \
		cmake --install build --prefix build/install
