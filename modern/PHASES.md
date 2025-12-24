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
- [x] Set up ImGui and SDL2/GLFW dependencies
- [x] Create main application window with OpenGL context
- [x] Implement basic menu bar (File, Edit, Image, Tools, Window)
- [x] Add application main loop with event handling

### 2.2 Canvas view and navigation (COMPLETED)
- [x] Integrate Canvas/Viewport with ImGui rendering
- [x] Implement zoom controls (zoom in/out, fit to window, 100%)
- [x] Add pan navigation (hand tool, spacebar modifier)
- [x] Display image coordinates and zoom level in status bar

**Implementation details:**
- Canvas window with OpenGL texture rendering integrated into main.cpp
- Zoom toolbar with buttons (Zoom In, Zoom Out, 100%, Fit) and keyboard shortcuts (Ctrl+Plus, Ctrl+Minus, Ctrl+0)
- Mouse wheel zoom with Ctrl modifier
- Pan navigation using spacebar+drag or middle mouse button
- Status bar displaying document size, image coordinates, and zoom level
- Tool integration with viewport coordinate transforms

### 2.3 Tool palette and status UI
- [x] Create tool palette window with tool icons
- [x] Implement tool selection and highlighting
- [x] Add tool options panel (size, hardness, opacity sliders)
- [x] Display status information (document size, memory usage)

## Phase 3: Tooling + editing workflows (planned)

### 3.1 Selection tools
- [x] Implement rectangular marquee tool
- [x] Implement elliptical marquee tool
- [x] Add lasso selection tool
- [x] Implement magic wand (color-based selection)
- [x] Add selection operations (invert, feather, grow/shrink)

### 3.2 Drawing tools
- [x] Implement brush tool with variable size/hardness
- [x] Add pencil tool (hard-edged drawing)
- [x] Create eraser tool
- [x] Implement paint bucket (fill) tool
- [x] Add eyedropper (color picker) tool

### 3.3 Channel and layer support (COMPLETED)
- [x] Extend ImageDocument for multiple layers
- [x] Implement layer blending modes
- [x] Add channel splitting/merging operations
- [x] Create layers panel UI

**Implementation details:**
- Created Layer class with RGBA8 buffer, opacity, visibility, and blend mode properties
- Implemented 12 blend modes (Normal, Multiply, Screen, Overlay, Darken, Lighten, Color Dodge, Color Burn, Hard Light, Soft Light, Difference, Exclusion)
- Extended ImageDocument with layer management methods (add_layer, remove_layer, move_layer, set_active_layer)
- Added layer-based rendering in Canvas with proper blend mode compositing
- Implemented channel splitting/merging operations for converting between layers and channels
- Created comprehensive Layers panel UI with:
  - Layer list showing all layers (top to bottom order)
  - Layer selection and activation
  - Visibility toggles
  - Opacity sliders
  - Blend mode selection
  - Layer name editing
  - New/Delete layer buttons
  - Convert channels to layer functionality

**Files created/modified:**
- `include/ps/core/layer.h` - Layer class definition
- `src/layer.cpp` - Layer implementation
- `include/ps/core/layer_blend.h` - Blend mode functions
- `src/layer_blend.cpp` - Blend mode implementations
- `include/ps/core/channel_operations.h` - Channel split/merge operations
- `src/channel_operations.cpp` - Channel operations implementation
- `include/ps/core/image_document.h` - Extended with layer support
- `src/image_document.cpp` - Layer management implementation
- `include/ps/rendering/canvas.h` - Added layer rendering support
- `src/rendering/canvas.cpp` - Layer compositing implementation
- `src/app/main.cpp` - Added Layers panel UI
- `CMakeLists.txt` - Added new source files

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
