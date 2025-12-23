# Photoshop 1.0 Source (Historical)

This repository contains a public mirror of the original Photoshop source code
for classic Macintosh systems.

Source origin:
http://computerhistory.org/atchm/adobe-photoshop-source-code/

## Modern Linux port (C++/ImGui)

A modern port is being bootstrapped under the `modern/` directory. The goal is
to preserve core behaviors while providing a native Linux UI built with C++ and
ImGui.

### Prerequisites

- CMake 3.16+
- A C++17-capable compiler (clang or gcc)
- libpng development headers
- SDL2 development headers
- OpenGL development headers

### Configure

```sh
cmake -S modern -B modern/build
```

### Build

```sh
cmake --build modern/build
```

### Run

The modern build produces both the core library and a minimal ImGui shell.

```sh
./modern/build/ps_modern_app
```

### Progress

See `modern/PHASES.md` for the planned phases and current progress.
