#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl2.h"

#include <memory>

#include "ps/core/image_document.h"
#include "ps/core/undo_stack.h"
#include "ps/rendering/canvas.h"
#include "ps/rendering/viewport.h"
#include "ps/tools/tool_manager.h"

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

// Canvas state
std::unique_ptr<ps::core::ImageDocument> g_document;
std::unique_ptr<ps::core::UndoStack> g_undo_stack;
std::unique_ptr<ps::rendering::Canvas> g_canvas;
ps::rendering::CanvasBuffer g_render_buffer;
GLuint g_texture_id = 0;
bool g_is_drawing = false;

void create_test_image() {
  g_document = std::make_unique<ps::core::ImageDocument>(
      ps::core::Size{800, 600}, ps::core::ColorMode::RGB);

  g_document->add_channel("Red", ps::core::PixelFormat::RGB8);
  g_document->add_channel("Green", ps::core::PixelFormat::RGB8);
  g_document->add_channel("Blue", ps::core::PixelFormat::RGB8);

  auto& red_channel = g_document->channel_at(0);
  auto& green_channel = g_document->channel_at(1);
  auto& blue_channel = g_document->channel_at(2);

  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 800; ++x) {
      const int idx = (y * 800 + x) * 3;
      red_channel.buffer.data()[idx] = static_cast<uint8_t>((x * 255) / 800);
      green_channel.buffer.data()[idx + 1] = static_cast<uint8_t>((y * 255) / 600);
      blue_channel.buffer.data()[idx + 2] = 128;
    }
  }

  g_canvas->viewport().set_viewport_size(ps::rendering::ViewportSize(800, 600));
  g_canvas->viewport().center_on_image(g_document->size());
}

void create_gl_texture() {
  if (g_texture_id != 0) return;

  glGenTextures(1, &g_texture_id);
  glBindTexture(GL_TEXTURE_2D, g_texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void update_canvas_texture() {
  if (g_render_buffer.pixels.empty()) return;

  if (g_texture_id == 0) {
    create_gl_texture();
  }

  glBindTexture(GL_TEXTURE_2D, g_texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_render_buffer.width,
               g_render_buffer.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               g_render_buffer.data());
  glBindTexture(GL_TEXTURE_2D, 0);
}

void cleanup_canvas() {
  if (g_texture_id != 0) {
    glDeleteTextures(1, &g_texture_id);
    g_texture_id = 0;
  }
  g_document.reset();
  g_undo_stack.reset();
  g_canvas.reset();
}
}

int main(int, char**) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_Window* window = SDL_CreateWindow(
      "Photoshop Modern",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      kWindowWidth,
      kWindowHeight,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL2_Init();

  // Initialize canvas system
  g_undo_stack = std::make_unique<ps::core::UndoStack>();
  g_canvas = std::make_unique<ps::rendering::Canvas>();
  ps::tools::ToolManager::instance().register_default_tools();
  create_test_image();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
    }

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("New...", nullptr, false, false);
        ImGui::MenuItem("Open...", nullptr, false, false);
        ImGui::MenuItem("Save", nullptr, false, false);
        ImGui::MenuItem("Save As...", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Quit", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Undo", nullptr, false, false);
        ImGui::MenuItem("Redo", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Cut", nullptr, false, false);
        ImGui::MenuItem("Copy", nullptr, false, false);
        ImGui::MenuItem("Paste", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Image")) {
        ImGui::MenuItem("Mode", nullptr, false, false);
        ImGui::MenuItem("Adjustments", nullptr, false, false);
        ImGui::MenuItem("Image Size...", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Tools")) {
        ImGui::MenuItem("Brush", nullptr, false, false);
        ImGui::MenuItem("Eraser", nullptr, false, false);
        ImGui::MenuItem("Marquee", nullptr, false, false);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Window")) {
        ImGui::MenuItem("Tools", nullptr, false, false);
        ImGui::MenuItem("Layers", nullptr, false, false);
        ImGui::MenuItem("History", nullptr, false, false);
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // Canvas window with zoom controls and rendering
    if (ImGui::Begin("Canvas", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
      // Toolbar with zoom controls
      ImGui::BeginGroup();
      if (ImGui::Button("Zoom In")) {
        g_canvas->viewport().zoom_in();
      }
      ImGui::SameLine();
      if (ImGui::Button("Zoom Out")) {
        g_canvas->viewport().zoom_out();
      }
      ImGui::SameLine();
      if (ImGui::Button("100%")) {
        g_canvas->viewport().set_zoom(1.0f);
        g_canvas->viewport().set_zoom_mode(ps::rendering::ZoomMode::ActualPixels);
      }
      ImGui::SameLine();
      if (ImGui::Button("Fit")) {
        if (g_document) {
          g_canvas->viewport().fit_to_window(g_document->size());
        }
      }
      ImGui::SameLine();
      ImGui::Text("Zoom: %.0f%%", g_canvas->viewport().zoom() * 100.0f);
      ImGui::EndGroup();

      // Canvas rendering area
      const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
      const ImVec2 canvas_size = ImGui::GetContentRegionAvail();

      if (canvas_size.x > 0 && canvas_size.y > 0 && g_document) {
        g_canvas->viewport().set_viewport_size(
            ps::rendering::ViewportSize(static_cast<int>(canvas_size.x),
                                       static_cast<int>(canvas_size.y)));

        g_render_buffer.resize(static_cast<int>(canvas_size.x),
                              static_cast<int>(canvas_size.y));

        g_canvas->render(*g_document, g_render_buffer);
        update_canvas_texture();

        if (g_texture_id) {
          ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(g_texture_id)),
                      canvas_size);
        }

        // Handle zoom keyboard shortcuts
        if (ImGui::IsWindowFocused()) {
          if (ImGui::IsKeyPressed(ImGuiKey_Equal) && io.KeyCtrl) {
            g_canvas->viewport().zoom_in();
          }
          if (ImGui::IsKeyPressed(ImGuiKey_Minus) && io.KeyCtrl) {
            g_canvas->viewport().zoom_out();
          }
          if (ImGui::IsKeyPressed(ImGuiKey_0) && io.KeyCtrl) {
            g_canvas->viewport().set_zoom(1.0f);
            g_canvas->viewport().set_zoom_mode(ps::rendering::ZoomMode::ActualPixels);
          }
        }

        // Handle mouse wheel zoom with Ctrl
        if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
          if (io.KeyCtrl) {
            if (io.MouseWheel > 0) {
              g_canvas->viewport().zoom_in();
            } else {
              g_canvas->viewport().zoom_out();
            }
          }
        }

        // Handle pan input (spacebar + drag or middle mouse)
        const bool is_space_down = ImGui::IsKeyDown(ImGuiKey_Space);
        const bool is_middle_mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

        if (ImGui::IsWindowHovered() && (is_space_down || is_middle_mouse_down)) {
          if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
              ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            const ImVec2 delta = ImGui::GetMouseDragDelta(
                is_space_down ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle);

            g_canvas->viewport().pan_by(delta.x, delta.y);

            ImGui::ResetMouseDragDelta(
                is_space_down ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle);
          }
        }

        // Handle tool input
        if (ImGui::IsWindowHovered() && !is_space_down) {
          ps::tools::Tool* active_tool = ps::tools::ToolManager::instance().active_tool();

          if (active_tool) {
            const ImVec2 mouse_pos = ImGui::GetMousePos();
            const ImVec2 window_pos = ImGui::GetWindowPos();
            const float title_bar_height = ImGui::GetFrameHeight();

            const ps::rendering::ViewportPoint vp(mouse_pos.x - canvas_pos.x,
                                                 mouse_pos.y - canvas_pos.y);
            const ps::rendering::ImagePoint ip = g_canvas->viewport().viewport_to_image(vp);

            const ps::tools::Point tool_pt(static_cast<int>(ip.x),
                                          static_cast<int>(ip.y));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
              active_tool->begin_stroke(*g_document, tool_pt);
              g_is_drawing = true;
            }

            if (g_is_drawing && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
              active_tool->continue_stroke(*g_document, tool_pt);
            }

            if (g_is_drawing && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
              auto cmd = active_tool->end_stroke(*g_document);
              if (cmd) {
                g_undo_stack->push(std::move(cmd));
              }
              g_is_drawing = false;
            }
          }
        }
      }

      // Status bar
      ImGui::Separator();
      if (g_document) {
        const ImVec2 mouse_pos = ImGui::GetMousePos();
        const ImVec2 window_pos = ImGui::GetWindowPos();
        const float title_bar_height = ImGui::GetFrameHeight();

        const ps::rendering::ViewportPoint vp(mouse_pos.x - window_pos.x,
                                             mouse_pos.y - window_pos.y - title_bar_height);
        const ps::rendering::ImagePoint ip = g_canvas->viewport().viewport_to_image(vp);

        ImGui::Text("Document: %dx%d", g_document->size().width, g_document->size().height);
        ImGui::SameLine();
        ImGui::Text("| Image: (%.0f, %.0f)", ip.x, ip.y);
        ImGui::SameLine();
        ImGui::Text("| Zoom: %.0f%%", g_canvas->viewport().zoom() * 100.0f);
      }
    }
    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  cleanup_canvas();

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
