#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "ps/core/command.h"
#include "ps/core/image_document.h"

namespace ps::tools {

/**
 * @brief Blend mode enumeration for painting and editing tools
 *
 * Defines how tool strokes combine with existing pixel data.
 */
enum class BlendMode {
  Normal,        ///< Standard alpha blending
  ColorOnly,     ///< Affects only color, preserves luminosity
  DarkenOnly,    ///< Only darkens existing pixels
  LightenOnly    ///< Only lightens existing pixels
};

/**
 * @brief Configuration parameters for tools
 *
 * Contains common settings shared by most painting and editing tools.
 * Tools can extend this structure or use only the relevant fields.
 */
struct ToolOptions {
  int size = 10;         ///< Brush/tool size in pixels
  int hardness = 100;    ///< Edge hardness (0=soft, 100=hard)
  int opacity = 100;     ///< Opacity percentage (0=transparent, 100=opaque)
  BlendMode blend_mode = BlendMode::Normal;  ///< How strokes blend with image

  int spacing = 25;      ///< Spacing between dabs (0-100, percentage of size)
  int fadeout = 0;       ///< Fadeout distance in pixels (0 = no fadeout)
};

/**
 * @brief A 2D point in image coordinates
 */
struct Point {
  int x = 0;  ///< X coordinate in pixels
  int y = 0;  ///< Y coordinate in pixels

  Point() = default;
  Point(int x_, int y_) : x(x_), y(y_) {}
};

/**
 * @brief A rectangular region in image coordinates
 */
struct Rect {
  int x = 0;       ///< Left edge X coordinate
  int y = 0;       ///< Top edge Y coordinate
  int width = 0;   ///< Width in pixels
  int height = 0;  ///< Height in pixels

  Rect() = default;
  Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

  /**
   * @brief Tests if a point is inside the rectangle
   * @param p Point to test
   * @return true if the point is inside or on the boundary
   */
  bool contains(Point p) const {
    return p.x >= x && p.x < x + width &&
           p.y >= y && p.y < y + height;
  }

  /**
   * @brief Tests if the rectangle is empty (zero or negative dimensions)
   * @return true if width or height is <= 0
   */
  bool is_empty() const {
    return width <= 0 || height <= 0;
  }
};

/**
 * @brief Abstract base class for all editing tools
 *
 * Tool defines the interface for interactive editing tools (brush, eraser,
 * selection tools, etc.). Tools operate via a stroke-based model:
 * 1. begin_stroke() - User presses mouse button
 * 2. continue_stroke() - User drags mouse (called repeatedly)
 * 3. end_stroke() - User releases mouse button, returns Command for undo
 *
 * This design mirrors the original Photoshop 1.0 tool architecture and
 * enables both direct painting and undoable operations.
 *
 * Example implementation:
 * @code
 *   class MyTool : public Tool {
 *     void begin_stroke(ImageDocument& doc, Point pt) override {
 *       // Initialize stroke
 *     }
 *     void continue_stroke(ImageDocument& doc, Point pt) override {
 *       // Apply incremental changes
 *     }
 *     std::unique_ptr<Command> end_stroke(ImageDocument& doc) override {
 *       return std::make_unique<MyCommand>(doc, ...);
 *     }
 *   };
 * @endcode
 */
class Tool {
 public:
  virtual ~Tool() = default;

  /**
   * @brief Returns the human-readable name of the tool
   * @return Tool name for UI display
   */
  virtual std::string name() const = 0;

  /**
   * @brief Returns a brief description of the tool
   * @return Description text, or empty string
   */
  virtual std::string description() const { return ""; }

  /**
   * @brief Returns the current tool options
   * @return Const reference to tool options
   */
  virtual const ToolOptions& options() const { return options_; }

  /**
   * @brief Updates the tool options
   * @param opts New options to apply
   */
  virtual void set_options(const ToolOptions& opts) { options_ = opts; }

  /**
   * @brief Begins a new stroke at the specified point
   * @param doc Document to modify
   * @param pt Starting point in image coordinates
   */
  virtual void begin_stroke(core::ImageDocument& doc, Point pt) = 0;

  /**
   * @brief Continues the current stroke to a new point
   * @param doc Document to modify
   * @param pt Current point in image coordinates
   */
  virtual void continue_stroke(core::ImageDocument& doc, Point pt) = 0;

  /**
   * @brief Ends the current stroke and returns an undo command
   * @param doc Document that was modified
   * @return Command for undoing the stroke, or nullptr if no undo needed
   */
  virtual std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) = 0;

  /**
   * @brief Indicates whether the tool requires an active document
   * @return true if the tool needs a document to operate
   */
  virtual bool requires_document() const { return true; }

 protected:
  ToolOptions options_;  ///< Current tool configuration
};

}  // namespace ps::tools
