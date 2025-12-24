#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ps/core/image_buffer.h"
#include "ps/core/layer.h"
#include "ps/core/selection_mask.h"

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

  /**
   * @brief Returns the current selection mask
   */
  const SelectionMask& selection() const;

  /**
   * @brief Returns a mutable reference to the selection mask
   */
  SelectionMask& selection();

  // Layer management methods

  /**
   * @brief Returns the number of layers in the document
   */
  std::size_t layer_count() const;

  /**
   * @brief Adds a new layer to the document
   * @param name Name for the new layer
   * @return Reference to the newly created layer
   */
  Layer& add_layer(const std::string& name = "Layer");

  /**
   * @brief Inserts a layer at a specific index
   * @param index Position to insert the layer (0 = bottom)
   * @param name Name for the new layer
   * @return Reference to the newly created layer
   */
  Layer& insert_layer(std::size_t index, const std::string& name = "Layer");

  /**
   * @brief Removes a layer at the specified index
   * @param index Index of the layer to remove
   */
  void remove_layer(std::size_t index);

  /**
   * @brief Returns a const reference to a layer by index
   * @param index Zero-based index (0 = bottom layer)
   * @return Const reference to the layer
   */
  const Layer& layer_at(std::size_t index) const;

  /**
   * @brief Returns a mutable reference to a layer by index
   * @param index Zero-based index (0 = bottom layer)
   * @return Reference to the layer
   */
  Layer& layer_at(std::size_t index);

  /**
   * @brief Returns a const reference to all layers
   */
  const std::vector<std::unique_ptr<Layer>>& layers() const;

  /**
   * @brief Moves a layer from one position to another
   * @param from_index Current index of the layer
   * @param to_index Destination index
   */
  void move_layer(std::size_t from_index, std::size_t to_index);

  /**
   * @brief Returns the index of the currently active layer
   */
  int active_layer_index() const { return active_layer_index_; }

  /**
   * @brief Sets the active layer by index
   */
  void set_active_layer(int index);

  /**
   * @brief Returns a pointer to the active layer, or nullptr if none
   */
  Layer* active_layer();

  /**
   * @brief Returns a const pointer to the active layer, or nullptr if none
   */
  const Layer* active_layer() const;

  /**
   * @brief Flattens all visible layers into the channel-based representation
   *
   * This composites all visible layers and updates the channel buffers.
   * Useful for operations that work with the legacy channel-based system.
   */
  void flatten_to_channels();

  /**
   * @brief Creates a layer from the current channel data
   *
   * Converts the channel-based image into a single layer.
   */
  void channels_to_layer(const std::string& name = "Background");

 private:
  Size size_{};
  ColorMode mode_ = ColorMode::RGB;
  std::vector<ImageChannel> channels_{};
  SelectionMask selection_{};
  std::vector<std::unique_ptr<Layer>> layers_{};
  int active_layer_index_ = -1;
};

}  // namespace ps::core
