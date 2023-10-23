# Vulkan Skeleton

A basic CMake setup for Vulkan development ☆彡

See Victor Blanco's [Vulkan Guide][vkguide-libs] for a list of libraries.

## Cloning

Most dependencies are included as submodules, so please clone this repo with:
```
git clone --recurse-submodules https://github.com/jakobbbb/hello-vulkan
```
or run `git submodule init && git submodule update` after cloning.

## Dependencies

Most external libraries are included as git submodules.  Install glfw3,
Vulkan itself and the necessary tools using your distro's package
manager, e.g.:


Ubuntu:
```
apt install build-essential \
  cmake \
  glslang-dev \
  glslang-tools \
  libglfw3-dev \
  libvulkan-dev \
  spirv-tools \
  vulkan-validationlayers-dev
```

Arch:
```
pacman -S \
    base-devel \
    cmake \
    glfw \
    glslang \
    vulkan-headers \
    vulkan-validation-layers
```

## Building

For convenience, a top-level `Makefile` is provided.
To build the entire project, simply run:
```
make
```

Binaries will be in `./build/source`.
Execute `make run` to run the main binary.

The following other Makefile targets may be of use:

* `build` (default)
* `debug`:
    Create a debug build.
* `test`:
    Compile and run tests.
* `lint`:
    Check code formatting with `clang-format`.
* `codeformat`:
    Auto-format code with `clang-format`.
* `clean`:
    Remove build files.

[vkguide-libs]:  https://vkguide.dev/docs/introduction/project_libs/
