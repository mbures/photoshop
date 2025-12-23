#include "ps/core/undo_stack.h"

#include <algorithm>

namespace ps::core {

UndoStack::UndoStack(std::size_t max_depth)
    : current_index_(0), max_depth_(max_depth) {
  commands_.reserve(max_depth);
}

void UndoStack::push(std::unique_ptr<Command> cmd) {
  if (!cmd) {
    return;
  }

  cmd->execute();

  if (current_index_ < commands_.size()) {
    commands_.erase(commands_.begin() + current_index_, commands_.end());
  }

  commands_.push_back(std::move(cmd));
  current_index_ = commands_.size();

  trim_to_max_depth();
}

void UndoStack::undo() {
  if (!can_undo()) {
    return;
  }

  --current_index_;
  commands_[current_index_]->undo();
}

void UndoStack::redo() {
  if (!can_redo()) {
    return;
  }

  commands_[current_index_]->redo();
  ++current_index_;
}

void UndoStack::clear() {
  commands_.clear();
  current_index_ = 0;
}

bool UndoStack::can_undo() const {
  return current_index_ > 0 && !commands_.empty();
}

bool UndoStack::can_redo() const {
  return current_index_ < commands_.size();
}

std::string UndoStack::undo_name() const {
  if (!can_undo()) {
    return "";
  }
  return commands_[current_index_ - 1]->name();
}

std::string UndoStack::redo_name() const {
  if (!can_redo()) {
    return "";
  }
  return commands_[current_index_]->name();
}

std::size_t UndoStack::undo_count() const { return current_index_; }

std::size_t UndoStack::redo_count() const {
  return commands_.size() - current_index_;
}

void UndoStack::set_max_depth(std::size_t depth) {
  max_depth_ = depth;
  trim_to_max_depth();
}

void UndoStack::trim_to_max_depth() {
  if (commands_.size() <= max_depth_) {
    return;
  }

  const std::size_t to_remove = commands_.size() - max_depth_;
  commands_.erase(commands_.begin(), commands_.begin() + to_remove);

  current_index_ = std::min(current_index_, commands_.size());
}

}  // namespace ps::core
