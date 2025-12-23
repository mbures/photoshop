# Modernization phases

This file tracks the agreed modernization phases and current progress for the
Linux C++/ImGui port.

## Phase 1: Core engine scaffolding (COMPLETED)

### 1.1 Core primitives (COMPLETED)
- [x] Establish core types (Size, PixelFormat, ColorMode)
- [x] Implement ImageBuffer with multiple pixel format support
- [x] Implement ImageDocument with channel management
- [x] Set up CMake build infrastructure with C++17

### 1.2 Command system & undo stack (COMPLETED)
- [x] Implement base Command class with execute/undo/redo interface
- [x] Implement UndoStack with configurable depth
- [x] Create ImageCommand base class for pixel operations
- [x] Add example commands (FillCommand, ClearCommand)

**Files created:**
- `include/ps/core/command.h`
- `include/ps/core/undo_stack.h`
- `src/command.cpp`
- `src/undo_stack.cpp`

### 1.3 Image I/O service (COMPLETED)
- [x] Implement ImageFormat abstract base class
- [x] Create PNGFormat concrete implementation (using libpng)
- [x] Implement ImageIO service with format registry
- [x] Integrate with ImageDocument for load/save operations
- [x] Add libpng dependency to CMakeLists.txt

**Files created:**
- `include/ps/io/image_format.h`
- `include/ps/io/png_format.h`
- `include/ps/io/image_io.h`
- `src/io/image_format.cpp`
- `src/io/png_format.cpp`
- `src/io/image_io.cpp`

### 1.4 Tool interface framework (COMPLETED)
- [x] Define Tool abstract base class with stroke methods
- [x] Create ToolOptions structure (size, hardness, opacity)
- [x] Implement BrushTool as concrete example
- [x] Create ToolManager for tool registration and switching

**Files created:**
- `include/ps/tools/tool.h`
- `include/ps/tools/brush_tool.h`
- `include/ps/tools/tool_manager.h`
- `src/tools/tool.cpp`
- `src/tools/brush_tool.cpp`
- `src/tools/tool_manager.cpp`

### 1.5 Canvas rendering hooks (COMPLETED)
- [x] Implement Viewport class for zoom/pan and coordinate transforms
- [x] Create Canvas class for rendering ImageDocument to RGBA buffer
- [x] Add support for multiple zoom levels (1:1, fit, custom)
- [x] Add selection overlay rendering infrastructure

**Files created:**
- `include/ps/rendering/canvas.h`
- `include/ps/rendering/viewport.h`
- `src/rendering/canvas.cpp`
- `src/rendering/viewport.cpp`

## Phase 2: Rendering + ImGui UI shell (NEXT)

### 2.1 ImGui application shell
- [ ] Set up ImGui and SDL2/GLFW dependencies
- [ ] Create main application window with OpenGL context
- [ ] Implement basic menu bar (File, Edit, Image, Tools, Window)
- [ ] Add application main loop with event handling

### 2.2 Canvas view and navigation
- [ ] Integrate Canvas/Viewport with ImGui rendering
- [ ] Implement zoom controls (zoom in/out, fit to window, 100%)
- [ ] Add pan navigation (hand tool, spacebar modifier)
- [ ] Display image coordinates and zoom level in status bar

### 2.3 Tool palette and status UI
- [ ] Create tool palette window with tool icons
- [ ] Implement tool selection and highlighting
- [ ] Add tool options panel (size, hardness, opacity sliders)
- [ ] Display status information (document size, memory usage)

## Phase 3: Tooling + editing workflows (planned)

### 3.1 Selection tools
- [ ] Implement rectangular marquee tool
- [ ] Implement elliptical marquee tool
- [ ] Add lasso selection tool
- [ ] Implement magic wand (color-based selection)
- [ ] Add selection operations (invert, feather, grow/shrink)

### 3.2 Drawing tools
- [ ] Implement brush tool with variable size/hardness
- [ ] Add pencil tool (hard-edged drawing)
- [ ] Create eraser tool
- [ ] Implement paint bucket (fill) tool
- [ ] Add eyedropper (color picker) tool

### 3.3 Channel and layer support
- [ ] Extend ImageDocument for multiple layers
- [ ] Implement layer blending modes
- [ ] Add channel splitting/merging operations
- [ ] Create layers panel UI

### 3.4 File format support
- [ ] Add TIFF format support (with LZW compression)
- [ ] Implement basic PSD format support (read/write)
- [ ] Add JPEG format support
- [ ] Support common formats (BMP, GIF)

## Phase 4: Parity + polish (planned)

### 4.1 Filters and adjustments
- [ ] Port blur/sharpen filters
- [ ] Implement brightness/contrast adjustments
- [ ] Add levels and curves controls
- [ ] Port color balance and hue/saturation

### 4.2 Performance optimization
- [ ] Profile rendering and identify bottlenecks
- [ ] Optimize buffer operations with SIMD
- [ ] Implement tiled rendering for large images
- [ ] Add memory usage monitoring and optimization

### 4.3 Quality assurance
- [ ] Create test suite with reference images
- [ ] Compare outputs against original Photoshop 1.0
- [ ] Fix visual discrepancies in rendering
- [ ] Document known limitations and differences

---

## Architecture Notes

### Original Pascal Architecture
The original Photoshop 1.0 codebase has four key architectural pillars:

1. **Command/Undo System** - Command pattern with buffer swapping for undo/redo
2. **Virtual Memory Manager** - Paged buffer system with disk backing (TVMArray)
3. **File I/O Framework** - Polymorphic format handlers (TImageFormat hierarchy)
4. **Tool Infrastructure** - Hierarchical tool classes (TMarkingTool, TDrawingTool)

### Modern C++ Port Strategy
The C++ port preserves these concepts while modernizing:

- Commands use RAII and smart pointers instead of manual memory management
- Virtual memory system may be deferred (modern systems have more RAM)
- Format handlers use standard C++ file I/O and third-party libraries
- Tools integrate with ImGui event system
- Namespace structure: `ps::core`, `ps::io`, `ps::tools`, `ps::rendering`

### Dependency Order
1. Core primitives (ImageBuffer, ImageDocument) - FOUNDATION
2. Command system - Required by tools and all edit operations
3. Image I/O - Enables testing with real images
4. Tool framework - Depends on commands for undo
5. Canvas/Viewport - Required for UI integration
6. ImGui shell - Brings everything together
