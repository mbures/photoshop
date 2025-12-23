# Modernization phases

This file tracks the agreed modernization phases and current progress for the
Linux C++/ImGui port.

## Phase 1: Core engine scaffolding (in progress)
- [x] Establish core types and primitives (ImageBuffer, ImageDocument).
- [ ] Add initial services (image I/O, undo stack, command model).
- [ ] Define tool interfaces and canvas rendering hooks.

## Phase 2: Rendering + ImGui UI shell (planned)
- [ ] Create ImGui application shell and main window layout.
- [ ] Add canvas view and basic navigation (zoom/pan).
- [ ] Integrate tool palette and status UI.

## Phase 3: Tooling + editing workflows (planned)
- [ ] Implement selection, brush, and basic drawing tools.
- [ ] Add channel/layer handling and history.
- [ ] Support core file formats for round-trip (PSD/TIFF/PNG).

## Phase 4: Parity + polish (planned)
- [ ] Expand filters and adjustments.
- [ ] Optimize performance and memory usage.
- [ ] QA against reference outputs from the original app.
