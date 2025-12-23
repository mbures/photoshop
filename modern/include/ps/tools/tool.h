#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "ps/core/command.h"
#include "ps/core/image_document.h"

namespace ps::tools {

enum class BlendMode {
  Normal,
  ColorOnly,
  DarkenOnly,
  LightenOnly
};

struct ToolOptions {
  int size = 10;
  int hardness = 100;    // 0-100
  int opacity = 100;     // 0-100
  BlendMode blend_mode = BlendMode::Normal;

  int spacing = 25;      // 0-100, for brush spacing
  int fadeout = 0;       // 0 = no fadeout
};

struct Point {
  int x = 0;
  int y = 0;

  Point() = default;
  Point(int x_, int y_) : x(x_), y(y_) {}
};

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  Rect() = default;
  Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

  bool contains(Point p) const {
    return p.x >= x && p.x < x + width &&
           p.y >= y && p.y < y + height;
  }

  bool is_empty() const {
    return width <= 0 || height <= 0;
  }
};

class Tool {
 public:
  virtual ~Tool() = default;

  virtual std::string name() const = 0;
  virtual std::string description() const { return ""; }

  virtual const ToolOptions& options() const { return options_; }
  virtual void set_options(const ToolOptions& opts) { options_ = opts; }

  virtual void begin_stroke(core::ImageDocument& doc, Point pt) = 0;
  virtual void continue_stroke(core::ImageDocument& doc, Point pt) = 0;
  virtual std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) = 0;

  virtual bool requires_document() const { return true; }

 protected:
  ToolOptions options_;
};

}  // namespace ps::tools
