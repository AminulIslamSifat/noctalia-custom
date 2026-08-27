#pragma once

#include "ui/controls/flex.h"
#include "ui/style.h"

#include <functional>

class InputArea;
class RectNode;

// A vertical slider — track runs top-to-bottom, value increases upward.
// Mirrors the horizontal Slider API but with Y-axis interaction.
class VerticalSlider : public Flex {
public:
  VerticalSlider();

  void setRange(double minValue, double maxValue);
  void setStep(double step);
  void setValue(double value);
  void setEnabled(bool enabled);
  void setTrackWidth(float width);
  void setThumbSize(float size);
  void setControlWidth(float width);
  void setWheelAdjustEnabled(bool enabled);
  void setOnValueChanged(std::function<void(double)> callback);
  void setOnDragEnd(std::function<void()> callback);

  [[nodiscard]] double value() const noexcept { return m_value; }
  [[nodiscard]] double minValue() const noexcept { return m_min; }
  [[nodiscard]] double maxValue() const noexcept { return m_max; }
  [[nodiscard]] bool enabled() const noexcept { return m_enabled; }
  [[nodiscard]] bool dragging() const noexcept;

private:
  void doLayout(Renderer& renderer) override;
  LayoutSize doMeasure(Renderer& renderer, const LayoutConstraints& constraints) override;
  void doArrange(Renderer& renderer, const LayoutRect& rect) override;
  void updateFromLocalY(float y);
  void updateGeometry();
  void applyVisualState();
  [[nodiscard]] float normalizedValue() const noexcept;
  [[nodiscard]] double snapped(double value) const noexcept;

  RectNode* m_track = nullptr;
  RectNode* m_fill = nullptr;
  RectNode* m_thumb = nullptr;
  InputArea* m_inputArea = nullptr;

  std::function<void(double)> m_onValueChanged;
  std::function<void()> m_onDragEnd;

  double m_min = 0.0;
  double m_max = 100.0;
  double m_step = 1.0;
  double m_value = 50.0;
  bool m_enabled = true;
  bool m_wheelAdjustEnabled = false;
  float m_trackWidth = Style::sliderTrackHeight; // reuse same constant
  float m_thumbSizePx = Style::sliderThumbSize;
  float m_controlWidthPx = Style::controlHeight; // reuse same constant
};
