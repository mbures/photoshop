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

### Configure

```sh
cmake -S modern -B modern/build
```

### Build

```sh
cmake --build modern/build
```

### Run

There is no runnable UI yet. The current output is a static library target
(`ps_modern_core`) intended for use by a future ImGui application.

### Progress

See `modern/PHASES.md` for the planned phases and current progress.
