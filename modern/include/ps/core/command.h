#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ps/core/image_document.h"

namespace ps::core {

/**
 * @brief Abstract base class for undoable operations
 *
 * Implements the Command pattern for all document-modifying operations.
 * Commands can be executed, undone, and redone. They are managed by the
 * UndoStack to provide unlimited undo/redo functionality.
 *
 * The command pattern is a core architectural element from the original
 * Photoshop 1.0, adapted here with modern C++ idioms (RAII, smart pointers).
 */
class Command {
 public:
  virtual ~Command() = default;

  /**
   * @brief Executes the command, applying changes to the document
   */
  virtual void execute() = 0;

  /**
   * @brief Undoes the command, reverting changes to the document
   */
  virtual void undo() = 0;

  /**
   * @brief Re-executes the command after it has been undone
   *
   * Default implementation calls execute(), but can be overridden
   * for commands that can optimize redo differently than execute.
   */
  virtual void redo() { execute(); }

  /**
   * @brief Returns a human-readable name for the command
   * @return Command name for display in undo/redo menu
   */
  virtual std::string name() const = 0;

  /**
   * @brief Indicates whether the command modifies the document
   * @return true if the command changes document state
   */
  virtual bool modifies_document() const { return true; }
};

/**
 * @brief Base class for commands that modify image channels
 *
 * ImageCommand extends Command with automatic channel backup and restore
 * functionality. It saves affected channels before modification and restores
 * them during undo operations.
 *
 * Derived classes should:
 * - Call save_channel() or save_all_channels() before modifying pixels
 * - Implement execute() to perform the actual pixel modifications
 */
class ImageCommand : public Command {
 public:
  /**
   * @brief Constructs a command that will operate on the given document
   * @param doc Reference to the image document to modify
   */
  explicit ImageCommand(ImageDocument& doc);
  ~ImageCommand() override = default;

  /**
   * @brief Restores saved channel data to undo the command
   */
  void undo() override;

 protected:
  ImageDocument& document_;  ///< The document being modified

  /**
   * @brief Stores a backup of a single channel
   */
  struct ChannelBackup {
    std::size_t index;   ///< Index of the backed-up channel
    ImageBuffer buffer;  ///< Copy of the channel's pixel data
  };
  std::vector<ChannelBackup> saved_channels_;  ///< List of backed-up channels

  /**
   * @brief Saves a copy of a single channel for later restoration
   * @param index Index of the channel to back up
   */
  void save_channel(std::size_t index);

  /**
   * @brief Saves copies of all channels in the document
   */
  void save_all_channels();

  /**
   * @brief Restores all saved channels to the document
   */
  void restore_channels();
};

/**
 * @brief Command that fills a channel with a solid color value
 */
class FillCommand : public ImageCommand {
 public:
  /**
   * @brief Constructs a fill command
   * @param doc Document to modify
   * @param channel_index Index of the channel to fill
   * @param value Byte value to fill (0-255)
   */
  FillCommand(ImageDocument& doc, std::size_t channel_index, std::uint8_t value);
  ~FillCommand() override = default;

  void execute() override;
  std::string name() const override { return "Fill"; }

 private:
  std::size_t channel_index_;
  std::uint8_t fill_value_;
};

/**
 * @brief Command that clears a channel to zero (black/transparent)
 */
class ClearCommand : public ImageCommand {
 public:
  /**
   * @brief Constructs a clear command
   * @param doc Document to modify
   * @param channel_index Index of the channel to clear
   */
  ClearCommand(ImageDocument& doc, std::size_t channel_index);
  ~ClearCommand() override = default;

  void execute() override;
  std::string name() const override { return "Clear"; }

 private:
  std::size_t channel_index_;
};

}  // namespace ps::core
