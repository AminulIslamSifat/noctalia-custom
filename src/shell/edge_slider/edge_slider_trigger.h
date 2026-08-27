#pragma once

#include "render/scene/input_dispatcher.h"
#include "wayland/layer_surface.h"

#include <functional>
#include <memory>
#include <vector>

class ConfigService;
class EdgeSliderPanel;
class Node;
class RenderContext;
class WaylandConnection;
struct PointerEvent;
struct wl_output;

// Invisible vertical strip on the right edge of each output that detects pointer
// hover and forwards enter/leave events to the EdgeSliderPanel.
class EdgeSliderTrigger {
public:
  EdgeSliderTrigger();
  ~EdgeSliderTrigger();

  EdgeSliderTrigger(const EdgeSliderTrigger&) = delete;
  EdgeSliderTrigger& operator=(const EdgeSliderTrigger&) = delete;

  void initialize(WaylandConnection& wayland, ConfigService* config, RenderContext* renderContext);
  void setPanel(EdgeSliderPanel* panel) { m_panel = panel; }
  void onOutputChange();
  bool onPointerEvent(const PointerEvent& event);

private:
  struct Instance {
    wl_output* output = nullptr;
    std::unique_ptr<LayerSurface> surface;
    std::unique_ptr<Node> sceneRoot;
    InputDispatcher inputDispatcher;
  };

  void ensureSurfaces();
  void destroySurfaces();

  WaylandConnection* m_wayland = nullptr;
  ConfigService* m_config = nullptr;
  RenderContext* m_renderContext = nullptr;
  EdgeSliderPanel* m_panel = nullptr;
  std::vector<std::unique_ptr<Instance>> m_instances;
};
