#pragma once

#include <cstdint>
#include <vector>

#include "ps/core/image_buffer.h"

namespace ps::core {

/**
 * @brief Alpha mask describing the current selection
 *
 * SelectionMask stores per-pixel selection strength (0-255) for the document.
 * Values greater than zero are considered selected.
 */
class SelectionMask {
 public:
  SelectionMask() = default;
  explicit SelectionMask(Size size);

  /**
   * @brief Resizes the selection mask and clears its contents
   * @param size New dimensions
   */
  void resize(Size size);

  /**
   * @brief Returns the current mask size
   */
  Size size() const { return size_; }

  /**
   * @brief Clears the selection mask (no selection)
   */
  void clear();

  /**
   * @brief Fills the mask with a uniform value
   * @param value Selection value (0-255)
   */
  void fill(std::uint8_t value);

  /**
   * @brief Returns the selection value at the given coordinates
   */
  std::uint8_t at(int x, int y) const;

  /**
   * @brief Sets the selection value at the given coordinates
   */
  void set(int x, int y, std::uint8_t value);

  /**
   * @brief Returns true if the given pixel is selected
   */
  bool is_selected(int x, int y) const { return at(x, y) > 0; }

  /**
   * @brief Returns true if any pixel is selected
   */
  bool has_selection() const;

  /**
   * @brief Fills a rectangular region with the given value
   */
  void fill_rect(int x, int y, int width, int height, std::uint8_t value = 255);

  /**
   * @brief Fills an elliptical region within the given bounds
   */
  void fill_ellipse(int x, int y, int width, int height, std::uint8_t value = 255);

  /**
   * @brief Inverts the selection (selected becomes unselected)
   */
  void invert();

  /**
   * @brief Softens selection edges by applying a blur radius
   */
  void feather(int radius);

  /**
   * @brief Expands the selection outward by the given radius
   */
  void grow(int radius);

  /**
   * @brief Contracts the selection inward by the given radius
   */
  void shrink(int radius);

 private:
  Size size_{};
  std::vector<std::uint8_t> mask_{};

  int index_for(int x, int y) const { return y * size_.width + x; }
  std::uint8_t at_unchecked(int x, int y) const {
    return mask_[index_for(x, y)];
  }
  void set_unchecked(int x, int y, std::uint8_t value) {
    mask_[index_for(x, y)] = value;
  }
};

}  // namespace ps::core
