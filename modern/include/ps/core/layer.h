#pragma once

#include <memory>
#include <string>

#include "ps/core/image_buffer.h"

namespace ps::core {

/**
 * @brief Blend modes for compositing layers
 *
 * Defines how a layer's pixels are combined with the layers below it.
 * Based on traditional Photoshop blend modes.
 */
enum class BlendMode {
  Normal,       ///< Standard alpha blending
  Multiply,     ///< Darkens by multiplying color values
  Screen,       ///< Lightens by inverting, multiplying, and inverting again
  Overlay,      ///< Combination of Multiply and Screen
  Darken,       ///< Keeps the darker of the two colors
  Lighten,      ///< Keeps the lighter of the two colors
  ColorDodge,   ///< Brightens by decreasing contrast
  ColorBurn,    ///< Darkens by increasing contrast
  HardLight,    ///< Similar to Overlay but with layer order reversed
  SoftLight,    ///< Softer version of HardLight
  Difference,   ///< Subtracts the darker from the lighter
  Exclusion     ///< Similar to Difference but lower contrast
};

/**
 * @brief Represents a single layer in an image document
 *
 * A layer contains image data (RGBA buffer), metadata (name, visibility),
 * and compositing properties (opacity, blend mode). Layers are composited
 * from bottom to top during rendering.
 */
class Layer {
 public:
  /**
   * @brief Constructs a layer with specified size and name
   * @param size Dimensions of the layer in pixels
   * @param name Human-readable name for the layer
   */
  explicit Layer(Size size, const std::string& name = "Layer");

  /**
   * @brief Returns the layer's name
   */
  const std::string& name() const { return name_; }

  /**
   * @brief Sets the layer's name
   */
  void set_name(const std::string& name) { name_ = name; }

  /**
   * @brief Returns the layer's visibility state
   */
  bool visible() const { return visible_; }

  /**
   * @brief Sets the layer's visibility
   */
  void set_visible(bool visible) { visible_ = visible; }

  /**
   * @brief Returns the layer's opacity (0-100)
   */
  int opacity() const { return opacity_; }

  /**
   * @brief Sets the layer's opacity (0-100)
   */
  void set_opacity(int opacity);

  /**
   * @brief Returns the layer's blend mode
   */
  BlendMode blend_mode() const { return blend_mode_; }

  /**
   * @brief Sets the layer's blend mode
   */
  void set_blend_mode(BlendMode mode) { blend_mode_ = mode; }

  /**
   * @brief Returns the layer's size
   */
  Size size() const { return size_; }

  /**
   * @brief Returns a const reference to the layer's image buffer
   *
   * The buffer stores RGBA8 pixel data for the entire layer.
   */
  const ImageBuffer& buffer() const { return buffer_; }

  /**
   * @brief Returns a mutable reference to the layer's image buffer
   */
  ImageBuffer& buffer() { return buffer_; }

 private:
  std::string name_;
  bool visible_ = true;
  int opacity_ = 100;  // 0-100
  BlendMode blend_mode_ = BlendMode::Normal;
  Size size_;
  ImageBuffer buffer_;
};

}  // namespace ps::core
