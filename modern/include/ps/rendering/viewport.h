#pragma once

#include <algorithm>
#include <cstddef>

#include "ps/core/image_document.h"

namespace ps::rendering {

/**
 * @brief A point in viewport (screen) coordinates
 */
struct ViewportPoint {
  float x = 0.0f;  ///< X coordinate in viewport pixels
  float y = 0.0f;  ///< Y coordinate in viewport pixels

  ViewportPoint() = default;
  ViewportPoint(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * @brief A point in image (document) coordinates
 */
struct ImagePoint {
  float x = 0.0f;  ///< X coordinate in image pixels
  float y = 0.0f;  ///< Y coordinate in image pixels

  ImagePoint() = default;
  ImagePoint(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * @brief Dimensions of the viewport window
 */
struct ViewportSize {
  int width = 0;   ///< Width in screen pixels
  int height = 0;  ///< Height in screen pixels

  ViewportSize() = default;
  ViewportSize(int w, int h) : width(w), height(h) {}
};

/**
 * @brief Zoom behavior mode for the viewport
 */
enum class ZoomMode {
  Custom,        ///< User-specified zoom level
  FitToWindow,   ///< Auto-zoom to fit image in window
  ActualPixels   ///< 1:1 pixel mapping (100% zoom)
};

/**
 * @brief Manages zoom, pan, and coordinate transformations for image viewing
 *
 * Viewport handles the mathematical transformations between screen coordinates
 * (where the user clicks) and image coordinates (where pixels are located).
 * It supports:
 * - Zoom levels from 0.01x to 64x (configurable range)
 * - Smooth zoom in/out with geometric progression
 * - Pan offset for scrolling around large images
 * - Multiple zoom modes (fit-to-window, 1:1, custom)
 * - Bidirectional coordinate transformation
 *
 * The viewport uses a simple affine transformation:
 *   viewport_coords = image_coords * zoom + pan_offset
 *
 * Example usage:
 * @code
 *   Viewport vp(ViewportSize{800, 600});
 *   vp.fit_to_window(Size{1920, 1080});  // Zoom to fit
 *   ImagePoint img = vp.viewport_to_image(ViewportPoint{400, 300});
 * @endcode
 */
class Viewport {
 public:
  /**
   * @brief Constructs a viewport with default size (800x600)
   */
  Viewport();

  /**
   * @brief Constructs a viewport with specified size
   * @param size Initial viewport dimensions
   */
  explicit Viewport(ViewportSize size);

  /**
   * @brief Sets the viewport window size
   * @param size New viewport dimensions
   */
  void set_viewport_size(ViewportSize size);

  /**
   * @brief Returns the current viewport size
   * @return Viewport dimensions
   */
  ViewportSize viewport_size() const { return viewport_size_; }

  /**
   * @brief Sets the zoom level directly
   * @param zoom Zoom factor (1.0 = 100%, 2.0 = 200%, etc.)
   */
  void set_zoom(float zoom);

  /**
   * @brief Returns the current zoom level
   * @return Zoom factor
   */
  float zoom() const { return zoom_; }

  /**
   * @brief Increases zoom by one step (multiplies by √2)
   */
  void zoom_in();

  /**
   * @brief Decreases zoom by one step (divides by √2)
   */
  void zoom_out();

  /**
   * @brief Sets the zoom mode
   * @param mode New zoom mode
   */
  void set_zoom_mode(ZoomMode mode);

  /**
   * @brief Returns the current zoom mode
   * @return Current zoom mode
   */
  ZoomMode zoom_mode() const { return zoom_mode_; }

  /**
   * @brief Sets the pan offset directly
   * @param x Horizontal pan offset in viewport pixels
   * @param y Vertical pan offset in viewport pixels
   */
  void set_pan(float x, float y);

  /**
   * @brief Adjusts the pan offset by a delta
   * @param dx Horizontal pan delta (positive = pan right)
   * @param dy Vertical pan delta (positive = pan down)
   */
  void pan_by(float dx, float dy);

  /**
   * @brief Returns the current pan offset
   * @return Pan offset in viewport coordinates
   */
  ViewportPoint pan_offset() const { return pan_offset_; }

  /**
   * @brief Centers the image in the viewport at the current zoom
   * @param image_size Dimensions of the image to center
   */
  void center_on_image(core::Size image_size);

  /**
   * @brief Zooms and pans to fit the entire image in the viewport
   * @param image_size Dimensions of the image to fit
   */
  void fit_to_window(core::Size image_size);

  /**
   * @brief Converts viewport coordinates to image coordinates
   * @param vp Point in viewport (screen) space
   * @return Corresponding point in image space
   */
  ImagePoint viewport_to_image(ViewportPoint vp) const;

  /**
   * @brief Converts image coordinates to viewport coordinates
   * @param ip Point in image space
   * @return Corresponding point in viewport (screen) space
   */
  ViewportPoint image_to_viewport(ImagePoint ip) const;

  /**
   * @brief Tests if an image point is visible in the viewport
   * @param ip Point in image space to test
   * @param image_size Dimensions of the image
   * @return true if the point is visible on screen
   */
  bool is_visible(ImagePoint ip, core::Size image_size) const;

  /**
   * @brief Resets viewport to default state (1:1 zoom, no pan)
   */
  void reset();

 private:
  ViewportSize viewport_size_;
  float zoom_ = 1.0f;
  ViewportPoint pan_offset_{0.0f, 0.0f};
  ZoomMode zoom_mode_ = ZoomMode::ActualPixels;

  void clamp_zoom();
  void update_zoom_for_fit(core::Size image_size);
};

}  // namespace ps::rendering
