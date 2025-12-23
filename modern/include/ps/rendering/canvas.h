#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ps/core/image_document.h"
#include "ps/rendering/viewport.h"

namespace ps::rendering {

struct RGBAPixel {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;

  RGBAPixel() = default;
  RGBAPixel(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_,
            std::uint8_t a_ = 255)
      : r(r_), g(g_), b(b_), a(a_) {}
};

struct CanvasBuffer {
  std::vector<RGBAPixel> pixels;
  int width = 0;
  int height = 0;

  CanvasBuffer() = default;
  CanvasBuffer(int w, int h) : width(w), height(h) {
    pixels.resize(w * h);
  }

  void resize(int w, int h) {
    width = w;
    height = h;
    pixels.resize(w * h);
  }

  void clear(RGBAPixel color = RGBAPixel(0, 0, 0, 0)) {
    std::fill(pixels.begin(), pixels.end(), color);
  }

  RGBAPixel& at(int x, int y) {
    return pixels[y * width + x];
  }

  const RGBAPixel& at(int x, int y) const {
    return pixels[y * width + x];
  }

  const std::uint8_t* data() const {
    return reinterpret_cast<const std::uint8_t*>(pixels.data());
  }

  std::uint8_t* data() {
    return reinterpret_cast<std::uint8_t*>(pixels.data());
  }

  std::size_t byte_size() const {
    return pixels.size() * sizeof(RGBAPixel);
  }
};

struct SelectionOverlay {
  bool enabled = false;
  RGBAPixel color{255, 255, 255, 128};
  int animation_frame = 0;
};

class Canvas {
 public:
  Canvas();
  explicit Canvas(const Viewport& viewport);

  void set_viewport(const Viewport& viewport) { viewport_ = viewport; }
  const Viewport& viewport() const { return viewport_; }
  Viewport& viewport() { return viewport_; }

  void render(const core::ImageDocument& doc, CanvasBuffer& buffer);

  void render_with_overlay(const core::ImageDocument& doc,
                          CanvasBuffer& buffer,
                          const SelectionOverlay& overlay);

  void set_background_color(RGBAPixel color) { background_color_ = color; }
  RGBAPixel background_color() const { return background_color_; }

  void set_checkerboard_enabled(bool enabled) {
    checkerboard_enabled_ = enabled;
  }
  bool checkerboard_enabled() const { return checkerboard_enabled_; }

 private:
  Viewport viewport_;
  RGBAPixel background_color_{128, 128, 128, 255};
  bool checkerboard_enabled_ = true;

  void render_background(CanvasBuffer& buffer);
  void render_checkerboard(CanvasBuffer& buffer, int checker_size = 8);
  void render_image(const core::ImageDocument& doc, CanvasBuffer& buffer);
  void render_selection_overlay(CanvasBuffer& buffer,
                                const SelectionOverlay& overlay);

  RGBAPixel sample_image(const core::ImageDocument& doc, float x, float y);
  RGBAPixel blend_pixels(RGBAPixel bottom, RGBAPixel top);
};

}  // namespace ps::rendering
