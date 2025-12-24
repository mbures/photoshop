#pragma once

#include <vector>

#include "ps/core/selection_command.h"
#include "ps/tools/tool.h"

namespace ps::tools {

/**
 * @brief Rectangular marquee selection tool
 */
class RectangularMarqueeTool : public Tool {
 public:
  std::string name() const override { return "Rect Marquee"; }
  std::string description() const override { return "Select rectangular areas"; }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  Point anchor_{};
  core::SelectionMask before_selection_{};
  bool stroke_active_ = false;

  void update_selection(core::ImageDocument& doc, Point pt);
};

/**
 * @brief Elliptical marquee selection tool
 */
class EllipticalMarqueeTool : public Tool {
 public:
  std::string name() const override { return "Ellipse Marquee"; }
  std::string description() const override { return "Select elliptical areas"; }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  Point anchor_{};
  core::SelectionMask before_selection_{};
  bool stroke_active_ = false;

  void update_selection(core::ImageDocument& doc, Point pt);
};

/**
 * @brief Freeform lasso selection tool
 */
class LassoSelectionTool : public Tool {
 public:
  std::string name() const override { return "Lasso"; }
  std::string description() const override { return "Draw freeform selections"; }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  std::vector<Point> points_{};
  core::SelectionMask before_selection_{};
  bool stroke_active_ = false;

  void update_selection(core::ImageDocument& doc);
};

/**
 * @brief Magic wand selection tool (color-based flood fill)
 */
class MagicWandTool : public Tool {
 public:
  std::string name() const override { return "Magic Wand"; }
  std::string description() const override { return "Select similar colors"; }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  core::SelectionMask before_selection_{};
  core::SelectionMask after_selection_{};
  bool stroke_active_ = false;
};

}  // namespace ps::tools
