#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ps/tools/tool.h"

namespace ps::tools {

/**
 * @brief Singleton manager for tool registration and selection
 *
 * ToolManager maintains a registry of available tools and tracks which
 * tool is currently active. It provides:
 * - Tool registration by string ID
 * - Active tool selection and retrieval
 * - Enumeration of available tools
 * - Default tool registration (brush, etc.)
 *
 * This is a singleton to ensure a single global tool palette across
 * the application. Tools are owned by the manager via unique_ptr.
 *
 * Example usage:
 * @code
 *   auto& mgr = ToolManager::instance();
 *   mgr.register_tool("brush", std::make_unique<BrushTool>());
 *   mgr.set_active_tool("brush");
 *   Tool* tool = mgr.active_tool();
 * @endcode
 */
class ToolManager {
 public:
  /**
   * @brief Returns the singleton instance of the tool manager
   * @return Reference to the global ToolManager instance
   */
  static ToolManager& instance();

  /**
   * @brief Registers a new tool with the manager
   * @param id String identifier for the tool (e.g., "brush", "eraser")
   * @param tool Unique pointer to the tool instance
   *
   * If this is the first tool registered, it becomes the active tool.
   * Ownership of the tool is transferred to the manager.
   */
  void register_tool(const std::string& id, std::unique_ptr<Tool> tool);

  /**
   * @brief Retrieves a tool by ID
   * @param id String identifier of the tool
   * @return Pointer to the tool, or nullptr if not found
   */
  Tool* get_tool(const std::string& id) const;

  /**
   * @brief Returns the currently active tool
   * @return Pointer to the active tool, or nullptr if no tool is active
   */
  Tool* active_tool() const { return active_tool_; }

  /**
   * @brief Sets the active tool by ID
   * @param id String identifier of the tool to activate
   *
   * Does nothing if the ID is not found in the registry.
   */
  void set_active_tool(const std::string& id);

  /**
   * @brief Returns a list of all registered tool IDs
   * @return Vector of tool ID strings
   */
  std::vector<std::string> tool_ids() const;

  /**
   * @brief Registers the default set of tools
   *
   * Currently registers:
   * - "brush" - BrushTool for painting
   *
   * Future versions will add more default tools.
   */
  void register_default_tools();

 private:
  ToolManager() = default;
  ~ToolManager() = default;

  ToolManager(const ToolManager&) = delete;
  ToolManager& operator=(const ToolManager&) = delete;

  std::unordered_map<std::string, std::unique_ptr<Tool>> tools_;
  Tool* active_tool_ = nullptr;
};

}  // namespace ps::tools
