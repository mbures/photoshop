#pragma once

#include <memory>
#include <vector>

#include "ps/tools/tool.h"

namespace ps::tools {

class BrushTool : public Tool {
 public:
  BrushTool();
  ~BrushTool() override = default;

  std::string name() const override { return "Brush"; }
  std::string description() const override {
    return "Paint with soft or hard-edged brush";
  }

  void begin_stroke(core::ImageDocument& doc, Point pt) override;
  void continue_stroke(core::ImageDocument& doc, Point pt) override;
  std::unique_ptr<core::Command> end_stroke(core::ImageDocument& doc) override;

 private:
  struct StrokePoint {
    Point position;
    int pressure = 100;  // 0-100
  };

  std::vector<StrokePoint> stroke_points_;
  Rect affected_area_;
  bool stroke_active_ = false;

  void apply_brush_dab(core::ImageDocument& doc, Point pt, int pressure);
  void expand_affected_area(Point pt);
  Rect calculate_dab_rect(Point pt) const;
};

}  // namespace ps::tools
