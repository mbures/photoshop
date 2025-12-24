#include "ps/core/selection_command.h"

namespace ps::core {

SelectionCommand::SelectionCommand(ImageDocument& doc,
                                   SelectionMask before,
                                   SelectionMask after,
                                   std::string label)
    : document_(doc),
      before_(std::move(before)),
      after_(std::move(after)),
      label_(std::move(label)) {}

void SelectionCommand::execute() {
  document_.selection() = after_;
}

void SelectionCommand::undo() {
  document_.selection() = before_;
}

}  // namespace ps::core
