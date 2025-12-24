#pragma once

#include <string>
#include <vector>

#include "ps/core/image_buffer.h"

namespace ps::core {

/**
 * @brief Color mode enumeration for image documents
 *
 * Defines the supported color modes, which determine how channels
 * are interpreted and combined during rendering.
 */
enum class ColorMode {
  Grayscale,  ///< Single channel grayscale image
  RGB,        ///< Three channel (Red, Green, Blue) color image
  CMYK        ///< Four channel (Cyan, Magenta, Yellow, Black) print image
};

/**
 * @brief Represents a single channel in an image document
 *
 * A channel is a named buffer of pixel data. In RGB mode, typical channels
 * are "Red", "Green", and "Blue". Additional channels can be added for
 * alpha masks or other purposes.
 */
struct ImageChannel {
  std::string name;   ///< Human-readable name of the channel
  ImageBuffer buffer; ///< Pixel data for this channel
};

/**
 * @brief An image document with multiple channels
 *
 * ImageDocument is the core data structure representing an editable image.
 * It contains multiple channels (e.g., RGB channels) and tracks the color
 * mode and dimensions.
 *
 * The document structure mirrors the original Photoshop 1.0 architecture,
 * where images are composed of separate channel buffers that are combined
 * during rendering.
 *
 * Example usage:
 * @code
 *   ImageDocument doc(Size{800, 600}, ColorMode::RGB);
 *   doc.add_channel("Red", PixelFormat::RGB8);
 *   doc.add_channel("Green", PixelFormat::RGB8);
 *   doc.add_channel("Blue", PixelFormat::RGB8);
 * @endcode
 */
class ImageDocument {
 public:
  /**
   * @brief Constructs an image document with specified size and color mode
   * @param size Dimensions of the document in pixels
   * @param mode Color mode (Grayscale, RGB, or CMYK)
   */
  explicit ImageDocument(Size size, ColorMode mode);

  /**
   * @brief Returns the dimensions of the document
   * @return Size structure with width and height
   */
  Size size() const;

  /**
   * @brief Returns the color mode of the document
   * @return Current color mode
   */
  ColorMode mode() const;

  /**
   * @brief Returns a const reference to all channels
   * @return Vector of image channels
   */
  const std::vector<ImageChannel>& channels() const;

  /**
   * @brief Returns a mutable reference to all channels
   * @return Vector of image channels
   */
  std::vector<ImageChannel>& channels();

  /**
   * @brief Adds a new channel to the document
   * @param name Human-readable name for the channel
   * @param format Pixel format for the channel buffer
   * @return Reference to the newly created channel
   */
  ImageChannel& add_channel(const std::string& name, PixelFormat format);

  /**
   * @brief Returns a mutable reference to a channel by index
   * @param index Zero-based index of the channel
   * @return Reference to the channel
   * @throw std::out_of_range if index is invalid
   */
  ImageChannel& channel_at(std::size_t index);

  /**
   * @brief Returns a const reference to a channel by index
   * @param index Zero-based index of the channel
   * @return Const reference to the channel
   * @throw std::out_of_range if index is invalid
   */
  const ImageChannel& channel_at(std::size_t index) const;

 private:
  Size size_{};
  ColorMode mode_ = ColorMode::RGB;
  std::vector<ImageChannel> channels_{};
};

}  // namespace ps::core
