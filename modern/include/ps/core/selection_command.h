#pragma once

#include <string>

#include "ps/core/command.h"
#include "ps/core/selection_mask.h"

namespace ps::core {

/**
 * @brief Command that captures selection mask changes for undo/redo
 */
class SelectionCommand : public Command {
 public:
  SelectionCommand(ImageDocument& doc,
                   SelectionMask before,
                   SelectionMask after,
                   std::string label);
  ~SelectionCommand() override = default;

  void execute() override;
  void undo() override;
  std::string name() const override { return label_; }

 private:
  ImageDocument& document_;
  SelectionMask before_;
  SelectionMask after_;
  std::string label_;
};

}  // namespace ps::core
