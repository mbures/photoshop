#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ps/core/image_document.h"

namespace ps::core {

class Command {
 public:
  virtual ~Command() = default;

  virtual void execute() = 0;
  virtual void undo() = 0;
  virtual void redo() { execute(); }

  virtual std::string name() const = 0;
  virtual bool modifies_document() const { return true; }
};

class ImageCommand : public Command {
 public:
  explicit ImageCommand(ImageDocument& doc);
  ~ImageCommand() override = default;

  void undo() override;

 protected:
  ImageDocument& document_;

  struct ChannelBackup {
    std::size_t index;
    ImageBuffer buffer;
  };
  std::vector<ChannelBackup> saved_channels_;

  void save_channel(std::size_t index);
  void save_all_channels();
  void restore_channels();
};

class FillCommand : public ImageCommand {
 public:
  FillCommand(ImageDocument& doc, std::size_t channel_index, std::uint8_t value);
  ~FillCommand() override = default;

  void execute() override;
  std::string name() const override { return "Fill"; }

 private:
  std::size_t channel_index_;
  std::uint8_t fill_value_;
};

class ClearCommand : public ImageCommand {
 public:
  ClearCommand(ImageDocument& doc, std::size_t channel_index);
  ~ClearCommand() override = default;

  void execute() override;
  std::string name() const override { return "Clear"; }

 private:
  std::size_t channel_index_;
};

}  // namespace ps::core
