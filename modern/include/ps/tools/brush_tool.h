#pragma once

#include <memory>
#include <vector>

#include "ps/tools/tool.h"

namespace ps::tools {

/**
 * @brief Command that captures and undoes a brush stroke
 *
 * BrushStrokeCommand saves all affected channels in the affected area
 * before the stroke is applied, allowing full undo/redo functionality.
 */
class BrushStrokeCommand : public core::ImageCommand {
 public:
  /**
   * @brief Constructs a brush stroke command
   * @param doc Document being modified
   * @param affected_area Rectangular region touched by the stroke
   */
  BrushStrokeCommand(core::ImageDocument& doc, const Rect& affected_area);
  ~BrushStrokeCommand() override = default;

  void execute() override;
  std::string name() const override { return "Brush Stroke"; }

 private:
  Rect affected_area_;  ///< Bounding box of the stroke
};

/**
 * @brief Painting tool with configurable size, hardness, and opacity
 *
 * BrushTool implements a basic painting brush similar to the original
 * Photoshop 1.0 brush. It supports:
 * - Variable brush size and hardness (soft/hard edges)
 * - Opacity control with alpha blending
 * - Pressure-sensitive painting (simulated)
 * - Circular brush shape with distance-based falloff
 * - Full undo/redo via BrushStrokeCommand
 *
 * The brush applies paint by placing overlapping circular "dabs" as the
 * user drags the mouse. Each dab is blended with existing pixels based
 * on opacity and hardness settings.
 *
 * @note Currently paints white; future versions will support color selection
 */
class BrushTool : public Tool {
 public:
  /**
   * @brief Constructs a brush tool with default settings
   *
   * Default settings: size=10, hardness=100, opacity=100
   */
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
  /**
   * @brief Records a point in the brush stroke with pressure information
   */
  struct StrokePoint {
    Point position;      ///< Location of the point
    int pressure = 100;  ///< Pressure value (0-100), affects size/opacity
  };

  std::vector<StrokePoint> stroke_points_;  ///< All points in current stroke
  Rect affected_area_;                      ///< Bounding box of modified pixels
  bool stroke_active_ = false;              ///< Whether a stroke is in progress
  std::unique_ptr<BrushStrokeCommand> current_command_;  ///< Command for current stroke

  /**
   * @brief Applies a single brush dab at the specified point
   * @param doc Document to paint on
   * @param pt Center point of the dab
   * @param pressure Pressure value (0-100) affecting dab intensity
   */
  void apply_brush_dab(core::ImageDocument& doc, Point pt, int pressure);

  /**
   * @brief Expands the affected area to include a new point
   * @param pt Point to include in the affected area
   */
  void expand_affected_area(Point pt);

  /**
   * @brief Calculates the bounding rectangle for a dab at the given point
   * @param pt Center point of the dab
   * @return Rectangle encompassing the dab area
   */
  Rect calculate_dab_rect(Point pt) const;
};

}  // namespace ps::tools
