#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ps/tools/tool.h"

namespace ps::tools {

/**
 * @brief Hard-edged drawing tool for precise lines
 */
class PencilTool : public Tool {
 public:
  PencilTool();
  ~PencilTool() override = default;

  std::string name() const override { return "Pencil"; }
  std::string description() const override {
    return "Draw hard-edged strokes";
  }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  Rect affected_area_{};
  bool stroke_active_ = false;
  std::unique_ptr<core::Command> current_command_;

  void apply_pencil_dab(core::ImageDocument& doc, Point pt);
  void expand_affected_area(Point pt);
  Rect calculate_dab_rect(Point pt) const;
};

/**
 * @brief Eraser tool for removing paint with soft or hard edges
 */
class EraserTool : public Tool {
 public:
  EraserTool();
  ~EraserTool() override = default;

  std::string name() const override { return "Eraser"; }
  std::string description() const override {
    return "Erase pixels with adjustable hardness";
  }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  Rect affected_area_{};
  bool stroke_active_ = false;
  std::unique_ptr<core::Command> current_command_;

  void apply_eraser_dab(core::ImageDocument& doc, Point pt);
  void expand_affected_area(Point pt);
  Rect calculate_dab_rect(Point pt) const;
};

/**
 * @brief Paint bucket tool for flood filling contiguous regions
 */
class PaintBucketTool : public Tool {
 public:
  PaintBucketTool();
  ~PaintBucketTool() override = default;

  std::string name() const override { return "Paint Bucket"; }
  std::string description() const override {
    return "Fill contiguous areas with paint";
  }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  bool stroke_active_ = false;
  std::unique_ptr<core::Command> current_command_;
};

/**
 * @brief Eyedropper tool for sampling colors from the canvas
 */
class EyedropperTool : public Tool {
 public:
  struct SampledColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
  };

  EyedropperTool() = default;
  ~EyedropperTool() override = default;

  std::string name() const override { return "Eyedropper"; }
  std::string description() const override { return "Sample colors from the image"; }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

  SampledColor sampled_color() const { return sampled_color_; }

 private:
  SampledColor sampled_color_{};
};

}  // namespace ps::tools
