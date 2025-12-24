#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ps/core/image_document.h"
#include "ps/core/selection_mask.h"
#include "ps/rendering/viewport.h"

namespace ps::rendering {

/**
 * @brief A single pixel in 32-bit RGBA format
 */
struct RGBAPixel {
  std::uint8_t r = 0;    ///< Red channel (0-255)
  std::uint8_t g = 0;    ///< Green channel (0-255)
  std::uint8_t b = 0;    ///< Blue channel (0-255)
  std::uint8_t a = 255;  ///< Alpha channel (0=transparent, 255=opaque)

  RGBAPixel() = default;
  RGBAPixel(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_,
            std::uint8_t a_ = 255)
      : r(r_), g(g_), b(b_), a(a_) {}
};

/**
 * @brief A buffer of RGBA pixels for on-screen display
 *
 * CanvasBuffer stores the final composited image ready for display
 * via OpenGL or other graphics APIs. It's always in RGBA format
 * regardless of the source document's color mode.
 */
struct CanvasBuffer {
  std::vector<RGBAPixel> pixels;  ///< Row-major pixel array
  int width = 0;                  ///< Width in pixels
  int height = 0;                 ///< Height in pixels

  CanvasBuffer() = default;
  CanvasBuffer(int w, int h) : width(w), height(h) {
    pixels.resize(w * h);
  }

  /**
   * @brief Resizes the buffer to new dimensions
   * @param w New width
   * @param h New height
   */
  void resize(int w, int h) {
    width = w;
    height = h;
    pixels.resize(w * h);
  }

  /**
   * @brief Fills the entire buffer with a solid color
   * @param color Fill color (default: transparent black)
   */
  void clear(RGBAPixel color = RGBAPixel(0, 0, 0, 0)) {
    std::fill(pixels.begin(), pixels.end(), color);
  }

  /**
   * @brief Returns a mutable reference to a pixel
   * @param x X coordinate
   * @param y Y coordinate
   * @return Reference to the pixel
   */
  RGBAPixel& at(int x, int y) {
    return pixels[y * width + x];
  }

  /**
   * @brief Returns a const reference to a pixel
   * @param x X coordinate
   * @param y Y coordinate
   * @return Const reference to the pixel
   */
  const RGBAPixel& at(int x, int y) const {
    return pixels[y * width + x];
  }

  /**
   * @brief Returns a const byte pointer to pixel data
   * @return Pointer suitable for passing to OpenGL
   */
  const std::uint8_t* data() const {
    return reinterpret_cast<const std::uint8_t*>(pixels.data());
  }

  /**
   * @brief Returns a mutable byte pointer to pixel data
   * @return Pointer suitable for passing to OpenGL
   */
  std::uint8_t* data() {
    return reinterpret_cast<std::uint8_t*>(pixels.data());
  }

  /**
   * @brief Returns the total size of the buffer in bytes
   * @return Buffer size (width × height × 4 bytes)
   */
  std::size_t byte_size() const {
    return pixels.size() * sizeof(RGBAPixel);
  }
};

/**
 * @brief Configuration for rendering selection outlines
 *
 * Used for "marching ants" animation around selected regions.
 * Currently a placeholder for future selection features.
 */
struct SelectionOverlay {
  bool enabled = false;                   ///< Whether overlay is visible
  RGBAPixel color{255, 255, 255, 128};    ///< Overlay color
  int animation_frame = 0;                ///< Animation frame counter
  const core::SelectionMask* mask = nullptr; ///< Selection mask to render
};

/**
 * @brief Renders image documents to screen buffers with viewport transformations
 *
 * Canvas combines an ImageDocument with a Viewport to produce an RGBA buffer
 * suitable for display. It handles:
 * - Background rendering (solid color or checkerboard for transparency)
 * - Color mode conversion (Grayscale, RGB, CMYK → RGBA)
 * - Zoom and pan via viewport transformations
 * - Alpha blending for transparent images
 * - Selection overlay rendering (marching ants)
 *
 * The rendering pipeline is:
 * 1. Render background (checkerboard or solid color)
 * 2. Composite image channels through viewport transform
 * 3. Optionally render selection overlay
 *
 * Example usage:
 * @code
 *   Canvas canvas;
 *   canvas.viewport().fit_to_window(doc.size());
 *   CanvasBuffer buffer(800, 600);
 *   canvas.render(doc, buffer);
 *   // Upload buffer.data() to OpenGL texture
 * @endcode
 */
class Canvas {
 public:
  /**
   * @brief Constructs a canvas with default viewport
   */
  Canvas();

  /**
   * @brief Constructs a canvas with specified viewport
   * @param viewport Initial viewport configuration
   */
  explicit Canvas(const Viewport& viewport);

  /**
   * @brief Sets the viewport for coordinate transformations
   * @param viewport New viewport configuration
   */
  void set_viewport(const Viewport& viewport) { viewport_ = viewport; }

  /**
   * @brief Returns a const reference to the viewport
   * @return Current viewport
   */
  const Viewport& viewport() const { return viewport_; }

  /**
   * @brief Returns a mutable reference to the viewport
   * @return Current viewport
   */
  Viewport& viewport() { return viewport_; }

  /**
   * @brief Renders a document to the canvas buffer
   * @param doc Source document to render
   * @param buffer Destination buffer (must be pre-sized)
   */
  void render(const core::ImageDocument& doc, CanvasBuffer& buffer);

  /**
   * @brief Renders a document with a selection overlay
   * @param doc Source document to render
   * @param buffer Destination buffer (must be pre-sized)
   * @param overlay Selection overlay configuration
   */
  void render_with_overlay(const core::ImageDocument& doc,
                          CanvasBuffer& buffer,
                          const SelectionOverlay& overlay);

  /**
   * @brief Sets the background color for areas outside the image
   * @param color New background color
   */
  void set_background_color(RGBAPixel color) { background_color_ = color; }

  /**
   * @brief Returns the current background color
   * @return Background color
   */
  RGBAPixel background_color() const { return background_color_; }

  /**
   * @brief Enables or disables checkerboard transparency background
   * @param enabled true to show checkerboard, false for solid color
   */
  void set_checkerboard_enabled(bool enabled) {
    checkerboard_enabled_ = enabled;
  }

  /**
   * @brief Returns whether checkerboard is enabled
   * @return true if checkerboard is shown
   */
  bool checkerboard_enabled() const { return checkerboard_enabled_; }

 private:
  Viewport viewport_;
  RGBAPixel background_color_{128, 128, 128, 255};
  bool checkerboard_enabled_ = true;

  void render_background(CanvasBuffer& buffer);
  void render_checkerboard(CanvasBuffer& buffer, int checker_size = 8);
  void render_image(const core::ImageDocument& doc, CanvasBuffer& buffer);
  void render_layers(const core::ImageDocument& doc, CanvasBuffer& buffer);
  void render_selection_overlay(CanvasBuffer& buffer,
                                const SelectionOverlay& overlay);

  RGBAPixel sample_image(const core::ImageDocument& doc, float x, float y);
  RGBAPixel sample_layer(const core::Layer& layer, float x, float y);
  RGBAPixel blend_pixels(RGBAPixel bottom, RGBAPixel top);
};

}  // namespace ps::rendering
