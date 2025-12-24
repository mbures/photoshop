# Modern Photoshop - Linux C++/ImGui Port

This directory contains a modern C++ port of Adobe Photoshop 1.0 for Linux, using ImGui for the user interface.

## Architecture Overview

The modern port preserves the core architectural patterns from the original Photoshop 1.0 while adapting them to modern C++ idioms and practices.

### Core Design Principles

1. **Channel-Based Image Representation** - Images are composed of separate channel buffers (R, G, B, etc.), mirroring the original design
2. **Command Pattern for Undo** - All modifications are encapsulated as commands with execute/undo/redo support
3. **Stroke-Based Tool Model** - Tools operate via begin/continue/end stroke lifecycle
4. **Separation of Concerns** - Clear boundaries between core logic, rendering, tools, and I/O

### Namespace Organization

The codebase is organized into four main namespaces:

```
ps::core       - Core data structures (ImageDocument, ImageBuffer, Command, UndoStack)
ps::tools      - Interactive editing tools (Tool, BrushTool, ToolManager)
ps::rendering  - Display and viewport management (Canvas, Viewport)
ps::io         - File format handling (ImageFormat, PNGFormat, ImageIO)
```

### Directory Structure

```
modern/
├── include/ps/          # Public headers
│   ├── core/            # Core image and command system
│   ├── tools/           # Tool framework and implementations
│   ├── rendering/       # Canvas and viewport
│   └── io/              # Image file I/O
├── src/                 # Implementation files
│   ├── app/             # Application entry point (main.cpp)
│   ├── tools/           # Tool implementations
│   ├── rendering/       # Rendering implementations
│   └── io/              # I/O format implementations
├── CMakeLists.txt       # Build configuration
└── PHASES.md            # Development roadmap and progress
```

## Key Components

### Image Document (`ps::core::ImageDocument`)

The central data structure representing an editable image.

- **Size and Color Mode** - Dimensions and color space (Grayscale, RGB, CMYK)
- **Channel-Based Storage** - Separate buffers for each color channel
- **Multi-Format Support** - Channels can have different pixel formats

```cpp
// Example: Create an RGB document
ImageDocument doc(Size{800, 600}, ColorMode::RGB);
doc.add_channel("Red", PixelFormat::RGB8);
doc.add_channel("Green", PixelFormat::RGB8);
doc.add_channel("Blue", PixelFormat::RGB8);
```

### Command System (`ps::core::Command`, `ps::core::UndoStack`)

Implements the Command pattern for all document modifications.

- **Command Interface** - `execute()`, `undo()`, `redo()`, `name()`
- **ImageCommand Base** - Automatic channel backup/restore for pixel operations
- **UndoStack** - Manages command history with configurable depth

**How it Works:**
1. Tools create commands when user interactions complete
2. Commands are pushed onto the UndoStack
3. Stack provides undo/redo functionality for the UI

```cpp
// Example: Using the undo stack
UndoStack stack;
auto cmd = std::make_unique<BrushStrokeCommand>(doc, affected_area);
stack.push(std::move(cmd));
stack.undo();  // Reverts the brush stroke
stack.redo();  // Reapplies it
```

### Tool Framework (`ps::tools::Tool`, `ps::tools::ToolManager`)

Stroke-based interface for interactive editing tools.

- **Stroke Lifecycle** - `begin_stroke()`, `continue_stroke()`, `end_stroke()`
- **Tool Options** - Configurable size, hardness, opacity, blend mode
- **Command Integration** - Tools return Commands for undo support

**Stroke Flow:**
1. User presses mouse → `begin_stroke()` is called
2. User drags mouse → `continue_stroke()` called repeatedly
3. User releases mouse → `end_stroke()` returns a Command

```cpp
// Example: Tool usage
auto& mgr = ToolManager::instance();
Tool* brush = mgr.active_tool();
brush->begin_stroke(doc, Point{100, 100});
brush->continue_stroke(doc, Point{105, 102});
auto cmd = brush->end_stroke(doc);  // Returns BrushStrokeCommand
```

### Rendering System (`ps::rendering::Canvas`, `ps::rendering::Viewport`)

Converts documents to displayable RGBA buffers with zoom/pan support.

**Viewport** - Manages coordinate transformations:
- Zoom levels from 0.01x to 64x
- Pan offset for navigation
- Bidirectional coordinate conversion (screen ↔ image)

**Canvas** - Renders documents to screen buffers:
- Background rendering (checkerboard or solid color)
- Color mode conversion (Grayscale/RGB/CMYK → RGBA)
- Alpha blending
- Selection overlays

```cpp
// Example: Rendering pipeline
Canvas canvas;
canvas.viewport().fit_to_window(doc.size());

CanvasBuffer buffer(800, 600);
canvas.render(doc, buffer);

// Upload buffer.data() to OpenGL texture for display
```

### File I/O (`ps::io::ImageFormat`, `ps::io::ImageIO`)

Extensible format handler system with plugin-style architecture.

- **ImageFormat Interface** - Abstract base for file formats
- **Format Registry** - Automatic format detection and registration
- **Current Formats** - PNG (read/write)

```cpp
// Example: Loading and saving
ImageIO io;
io.register_format(std::make_unique<PNGFormat>());

auto doc = io.load("input.png");
io.save(*doc, "output.png");
```

## Development Phases

The port is being developed in phases. See [PHASES.md](PHASES.md) for details.

**Completed:**
- ✅ Phase 1: Core engine scaffolding (commands, channels, tools, rendering)
- ✅ Phase 2: ImGui UI shell (canvas view, tool palette, zoom/pan)

**In Progress:**
- 🚧 Phase 3: Tooling + editing workflows (selection tools, drawing tools, layers)

**Planned:**
- 📋 Phase 4: Parity + polish (filters, optimizations, quality assurance)

## Building

### Prerequisites

- CMake 3.16+
- C++17-capable compiler (GCC 7+ or Clang 5+)
- libpng development headers
- SDL2 development headers
- OpenGL development headers

### Ubuntu/Debian

```bash
sudo apt-get install cmake build-essential libpng-dev libsdl2-dev libgl1-mesa-dev
```

### Build Steps

```bash
# Configure
cmake -S modern -B modern/build

# Build
cmake --build modern/build

# Run
./modern/build/ps_modern_app
```

## Design Decisions

### Why Channels Instead of Pixels?

The original Photoshop used channel-based storage for several reasons:
1. **Memory Efficiency** - Channels can be compressed or paged to disk independently
2. **Flexible Color Modes** - Easy to support RGB, CMYK, LAB, etc.
3. **Channel Operations** - Operations like "blur red channel" are natural
4. **Historical Accuracy** - Matches the original architecture

### Why Command Pattern?

The Command pattern provides:
1. **Unlimited Undo** - Limited only by memory, not by implementation complexity
2. **Macro Recording** - Commands can be recorded and replayed
3. **Testing** - Each operation is a discrete, testable unit
4. **Extensibility** - New operations just need to implement the interface

### Why Stroke-Based Tools?

Stroke-based tools provide:
1. **Incremental Feedback** - User sees changes as they drag
2. **Efficient Undo** - One command per stroke, not per pixel
3. **Pressure Support** - Natural integration with tablet pressure curves
4. **Tool Simplicity** - Tools focus on pixel modifications, not undo logic

## Adding New Features

### Adding a New Tool

1. Create header in `include/ps/tools/your_tool.h`:
   ```cpp
   class YourTool : public Tool {
     void begin_stroke(ImageDocument& doc, Point pt) override;
     void continue_stroke(ImageDocument& doc, Point pt) override;
     std::unique_ptr<Command> end_stroke(ImageDocument& doc) override;
   };
   ```

2. Implement in `src/tools/your_tool.cpp`

3. Register in `ToolManager::register_default_tools()`:
   ```cpp
   register_tool("your_tool", std::make_unique<YourTool>());
   ```

### Adding a New Command

1. Create class inheriting from `ImageCommand`:
   ```cpp
   class MyCommand : public ImageCommand {
     void execute() override {
       save_all_channels();  // Save before modifying
       // Modify document here
     }
   };
   ```

2. Commands automatically support undo via `ImageCommand::undo()`

### Adding a File Format

1. Create class inheriting from `ImageFormat`:
   ```cpp
   class TIFFFormat : public ImageFormat {
     bool can_read(const std::string& path) const override;
     std::unique_ptr<ImageDocument> read(const std::string& path) override;
     bool can_write(const ImageDocument& doc) const override;
     void write(const ImageDocument& doc, const std::string& path) override;
   };
   ```

2. Register with `ImageIO`:
   ```cpp
   io.register_format(std::make_unique<TIFFFormat>());
   ```

## Code Style Guidelines

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `ImageDocument`, `BrushTool`)
- **Functions/Methods**: `snake_case` (e.g., `add_channel()`, `begin_stroke()`)
- **Member Variables**: `snake_case_` with trailing underscore (e.g., `size_`, `channels_`)
- **Constants**: `kPascalCase` (e.g., `kMaxZoom`, `kDefaultSize`)

### Documentation

- All public classes and methods should have Doxygen comments
- Use `@brief` for short descriptions
- Use `@param` and `@return` for method parameters
- Include usage examples for complex APIs

### Modern C++ Practices

- Use `std::unique_ptr` for ownership
- Use `const` liberally
- Prefer `override` to `virtual` in derived classes
- Use RAII for resource management
- Avoid raw `new`/`delete`

## Testing

Currently, testing is manual via the ImGui application. Future plans include:

- Unit tests for core classes
- Reference image comparison tests
- Performance benchmarks
- Fuzz testing for file I/O

## Performance Considerations

### Current Approach

- **Simple Loops** - Direct pixel manipulation in C++
- **No SIMD** - Straightforward, portable code
- **Full Rerender** - Canvas is redrawn every frame

### Future Optimizations

- **SIMD Vectorization** - Use SSE/AVX for pixel operations
- **Tiled Rendering** - Only redraw changed regions
- **GPU Acceleration** - Offload filters to shaders
- **Memory Pooling** - Reduce allocations for temp buffers

## Differences from Original

### Modernizations

1. **Memory Management** - RAII and smart pointers vs manual memory
2. **Standard Library** - `std::vector`, `std::string` vs MacApp types
3. **File I/O** - Standard C++ streams vs Mac Toolbox
4. **Threading** - std::thread available for future use

### Not Yet Implemented

1. **Virtual Memory System** - Original had disk-backed paging (defer until needed)
2. **Scripting** - No plugin/macro system yet
3. **Printer Support** - Focus on screen display first
4. **Multiple Windows** - Single document for now

## License

This modern port follows the licensing of the original Photoshop 1.0 source code.
See the root README.md for details on the historical source code license.

## Contributing

When contributing:

1. Follow the existing code style
2. Add documentation for new public APIs
3. Update PHASES.md if adding new features
4. Test your changes with the ImGui application
5. Ensure code compiles with zero warnings

## Resources

- [Original Photoshop 1.0 Source](http://computerhistory.org/atchm/adobe-photoshop-source-code/)
- [ImGui Documentation](https://github.com/ocornut/imgui)
- [SDL2 Documentation](https://wiki.libsdl.org/)
- [PHASES.md](PHASES.md) - Development roadmap

## Contact

For questions about this modern port, please file an issue in the repository.
