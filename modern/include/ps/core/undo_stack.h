#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ps/core/command.h"

namespace ps::core {

class UndoStack {
 public:
  explicit UndoStack(std::size_t max_depth = 100);

  void push(std::unique_ptr<Command> cmd);
  void undo();
  void redo();
  void clear();

  bool can_undo() const;
  bool can_redo() const;

  std::string undo_name() const;
  std::string redo_name() const;

  std::size_t undo_count() const;
  std::size_t redo_count() const;

  std::size_t max_depth() const { return max_depth_; }
  void set_max_depth(std::size_t depth);

 private:
  std::vector<std::unique_ptr<Command>> commands_;
  std::size_t current_index_;
  std::size_t max_depth_;

  void trim_to_max_depth();
};

}  // namespace ps::core
