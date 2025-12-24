#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ps::core {

/**
 * @brief Represents the dimensions of an image or buffer
 */
struct Size {
  int width = 0;   ///< Width in pixels
  int height = 0;  ///< Height in pixels
};

/**
 * @brief Pixel format enumeration for image buffers
 *
 * Defines the supported pixel formats with their bit depths.
 * All formats currently use 8 bits per channel.
 */
enum class PixelFormat {
  Gray8,   ///< 8-bit grayscale (1 byte per pixel)
  RGB8,    ///< 8-bit RGB (3 bytes per pixel)
  RGBA8,   ///< 8-bit RGBA (4 bytes per pixel)
  CMYK8    ///< 8-bit CMYK (4 bytes per pixel)
};

/**
 * @brief Returns the number of bytes per pixel for a given format
 * @param format The pixel format to query
 * @return Number of bytes per pixel (1, 3, or 4)
 */
std::size_t bytes_per_pixel(PixelFormat format);

/**
 * @brief A buffer for storing pixel data in various formats
 *
 * ImageBuffer manages a contiguous block of memory for storing pixel data.
 * It supports multiple pixel formats (grayscale, RGB, RGBA, CMYK) and
 * handles memory allocation and resizing automatically.
 *
 * Memory layout is row-major: pixels are stored left-to-right, top-to-bottom.
 * For multi-channel formats, channels are interleaved (e.g., RGBRGBRGB...).
 *
 * Example usage:
 * @code
 *   ImageBuffer buffer(Size{640, 480}, PixelFormat::RGB8);
 *   std::uint8_t* pixels = buffer.data();
 *   // Access pixel at (x, y): pixels[(y * 640 + x) * 3]
 * @endcode
 */
class ImageBuffer {
 public:
  /**
   * @brief Constructs an empty buffer with zero size
   */
  ImageBuffer() = default;

  /**
   * @brief Constructs a buffer with specified size and format
   * @param size Dimensions of the buffer
   * @param format Pixel format for the buffer
   */
  ImageBuffer(Size size, PixelFormat format);

  /**
   * @brief Returns the dimensions of the buffer
   * @return Size structure with width and height
   */
  Size size() const;

  /**
   * @brief Returns the pixel format of the buffer
   * @return Current pixel format
   */
  PixelFormat format() const;

  /**
   * @brief Returns a mutable pointer to the pixel data
   * @return Pointer to the first byte of pixel data
   */
  std::uint8_t* data();

  /**
   * @brief Returns a const pointer to the pixel data
   * @return Const pointer to the first byte of pixel data
   */
  const std::uint8_t* data() const;

  /**
   * @brief Returns the total size of the buffer in bytes
   * @return Size in bytes (width × height × bytes_per_pixel)
   */
  std::size_t byte_size() const;

  /**
   * @brief Resizes the buffer and changes its format
   * @param size New dimensions for the buffer
   * @param format New pixel format for the buffer
   * @note Existing pixel data is discarded
   */
  void resize(Size size, PixelFormat format);

 private:
  Size size_{};
  PixelFormat format_ = PixelFormat::RGB8;
  std::vector<std::uint8_t> pixels_{};
};

}  // namespace ps::core
