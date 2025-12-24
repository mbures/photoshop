#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "ps/core/command.h"

namespace ps::core {

/**
 * @brief Manages a stack of undoable commands with configurable depth
 *
 * UndoStack maintains a linear history of executed commands and provides
 * undo/redo functionality. It implements a classic undo stack with:
 * - Configurable maximum depth to limit memory usage
 * - Linear history (undoing then executing a new command clears redo history)
 * - Command name queries for UI display
 *
 * The stack maintains an internal index pointing to the current position
 * in the command history. Commands before the index can be undone, and
 * commands at or after the index can be redone.
 *
 * Example usage:
 * @code
 *   UndoStack stack(50);  // Max 50 levels of undo
 *   stack.push(std::make_unique<FillCommand>(...));
 *   stack.undo();  // Undo the fill
 *   stack.redo();  // Redo the fill
 * @endcode
 */
class UndoStack {
 public:
  /**
   * @brief Constructs an undo stack with specified maximum depth
   * @param max_depth Maximum number of commands to keep (default: 100)
   */
  explicit UndoStack(std::size_t max_depth = 100);

  /**
   * @brief Pushes a new command onto the stack and executes it
   * @param cmd Unique pointer to the command to execute
   *
   * This clears any redo history and executes the command. If the stack
   * exceeds max_depth, the oldest command is discarded.
   */
  void push(std::unique_ptr<Command> cmd);

  /**
   * @brief Undoes the most recently executed command
   *
   * Moves the current index back and calls undo() on the command.
   * Does nothing if there are no commands to undo.
   */
  void undo();

  /**
   * @brief Redoes the most recently undone command
   *
   * Moves the current index forward and calls redo() on the command.
   * Does nothing if there are no commands to redo.
   */
  void redo();

  /**
   * @brief Clears all commands from the stack
   *
   * Resets the stack to its initial empty state. This cannot be undone.
   */
  void clear();

  /**
   * @brief Checks if an undo operation is possible
   * @return true if there are commands that can be undone
   */
  bool can_undo() const;

  /**
   * @brief Checks if a redo operation is possible
   * @return true if there are commands that can be redone
   */
  bool can_redo() const;

  /**
   * @brief Returns the name of the command that would be undone
   * @return Command name, or empty string if no undo is available
   */
  std::string undo_name() const;

  /**
   * @brief Returns the name of the command that would be redone
   * @return Command name, or empty string if no redo is available
   */
  std::string redo_name() const;

  /**
   * @brief Returns the number of commands that can be undone
   * @return Count of undoable commands
   */
  std::size_t undo_count() const;

  /**
   * @brief Returns the number of commands that can be redone
   * @return Count of redoable commands
   */
  std::size_t redo_count() const;

  /**
   * @brief Returns the maximum depth of the stack
   * @return Maximum number of commands that can be stored
   */
  std::size_t max_depth() const { return max_depth_; }

  /**
   * @brief Sets the maximum depth of the stack
   * @param depth New maximum depth
   *
   * If the new depth is less than the current number of commands,
   * the oldest commands are discarded.
   */
  void set_max_depth(std::size_t depth);

 private:
  std::vector<std::unique_ptr<Command>> commands_;
  std::size_t current_index_;
  std::size_t max_depth_;

  void trim_to_max_depth();
};

}  // namespace ps::core
