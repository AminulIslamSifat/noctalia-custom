#include "ui/controls/vertical_slider.h"

#include "core/input/key_symbols.h"
#include "cursor-shape-v1-client-protocol.h"
#include "render/core/render_styles.h"
#include "render/scene/input_area.h"
#include "render/scene/rect_node.h"
#include "ui/palette.h"
#include "ui/style.h"
#include "util/clamp.h"

#include <algorithm>
#include <cmath>
#include <linux/input-event-codes.h>
#include <memory>

namespace {
  constexpr double kValueEpsilon = 0.0001;

  RoundedRectStyle solidStyle(const Color& fill, float radius) {
    return RoundedRectStyle{
        .fill = fill,
        .border = fill,
        .fillMode = FillMode::Solid,
        .radius = radius,
        .softness = 1.0F,
        .borderWidth = 0.0F,
    };
  }

  Color resolved(ColorRole role, float alpha = 1.0F) { return colorForRole(role, alpha); }
} // namespace

VerticalSlider::VerticalSlider() {
  auto track = std::make_unique<RectNode>();
  m_track = static_cast<RectNode*>(addChild(std::move(track)));

  auto fill = std::make_unique<RectNode>();
  m_fill = static_cast<RectNode*>(addChild(std::move(fill)));

  auto thumb = std::make_unique<RectNode>();
  m_thumb = static_cast<RectNode*>(addChild(std::move(thumb)));

  auto area = std::make_unique<InputArea>();
  area->setOnEnter([this](const InputArea::PointerData&) {
    applyVisualState();
    markPaintDirty();
  });
  area->setOnLeave([this]() {
    applyVisualState();
    markPaintDirty();
  });
  area->setOnPress([this](const InputArea::PointerData& data) {
    if (!m_enabled || data.button != BTN_LEFT) return;
    if (!data.pressed) {
      applyVisualState();
      markPaintDirty();
      if (m_onDragEnd) m_onDragEnd();
      return;
    }
    applyVisualState();
    updateFromLocalY(data.localY);
    markPaintDirty();
  });
  area->setOnMotion([this](const InputArea::PointerData& data) {
    if (!m_enabled || m_inputArea == nullptr || !m_inputArea->pressed()) return;
    updateFromLocalY(data.localY);
  });
  area->setOnAxisHandler([this](const InputArea::PointerData& data) {
    if (!m_enabled || !m_wheelAdjustEnabled || data.axis != WL_POINTER_AXIS_VERTICAL_SCROLL) {
      return false;
    }
    const auto lines = static_cast<double>(data.scrollSteps());
    if (lines == 0.0) return false;
    const double step = m_step > 0.0 ? m_step : (m_max - m_min) * 0.05;
    if (step <= 0.0) return false;
    // Scroll up = increase value
    setValue(m_value - lines * step);
    if (m_onDragEnd) m_onDragEnd();
    return true;
  });
  area->setFocusable(true);
  area->setOnFocusGain([this]() { applyVisualState(); markPaintDirty(); });
  area->setOnFocusLoss([this]() { applyVisualState(); markPaintDirty(); });
  area->setOnKeyDown([this](const InputArea::KeyData& key) {
    if (!key.pressed || !m_enabled) return;
    const double step = m_step > 0.0 ? m_step : (m_max - m_min) * 0.05;
    if (step <= 0.0) return;
    if (KeySymbol::isPageUp(key.sym)) {
      setValue(m_value + step * 10.0);
      if (m_onDragEnd) m_onDragEnd();
    } else if (KeySymbol::isPageDown(key.sym)) {
      setValue(m_value - step * 10.0);
      if (m_onDragEnd) m_onDragEnd();
    } else if (KeySymbol::isHome(key.sym)) {
      setValue(m_max);
      if (m_onDragEnd) m_onDragEnd();
    } else if (KeySymbol::isEnd(key.sym)) {
      setValue(m_min);
      if (m_onDragEnd) m_onDragEnd();
    }
  });
  m_inputArea = static_cast<InputArea*>(addChild(std::move(area)));
  m_inputArea->setCursorShape(WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER);

  applyVisualState();
}

void VerticalSlider::setRange(double minValue, double maxValue) {
  if (maxValue < minValue) std::swap(minValue, maxValue);
  if (m_min == minValue && m_max == maxValue) return;
  m_min = minValue;
  m_max = maxValue;
  const double next = snapped(m_value);
  const bool changed = std::abs(next - m_value) >= kValueEpsilon;
  m_value = next;
  updateGeometry();
  markPaintDirty();
  if (changed && m_onValueChanged) m_onValueChanged(m_value);
}

void VerticalSlider::setStep(double step) {
  m_step = std::max(step, 0.0);
  const double next = snapped(m_value);
  const bool changed = std::abs(next - m_value) >= kValueEpsilon;
  m_value = next;
  updateGeometry();
  markPaintDirty();
  if (changed && m_onValueChanged) m_onValueChanged(m_value);
}

void VerticalSlider::setValue(double value) {
  const double next = snapped(value);
  if (std::abs(next - m_value) < kValueEpsilon) return;
  m_value = next;
  updateGeometry();
  markPaintDirty();
  if (m_onValueChanged) m_onValueChanged(m_value);
}

void VerticalSlider::setEnabled(bool enabled) {
  if (m_enabled == enabled) return;
  m_enabled = enabled;
  applyVisualState();
  markPaintDirty();
}

void VerticalSlider::setTrackWidth(float width) {
  m_trackWidth = std::max(1.0F, width);
  updateGeometry();
  markLayoutDirty();
}

void VerticalSlider::setThumbSize(float size) {
  m_thumbSizePx = std::max(1.0F, size);
  updateGeometry();
  markLayoutDirty();
}

void VerticalSlider::setControlWidth(float width) {
  m_controlWidthPx = std::max(1.0F, width);
  updateGeometry();
  markLayoutDirty();
}

void VerticalSlider::setWheelAdjustEnabled(bool enabled) { m_wheelAdjustEnabled = enabled; }
void VerticalSlider::setOnValueChanged(std::function<void(double)> cb) { m_onValueChanged = std::move(cb); }
void VerticalSlider::setOnDragEnd(std::function<void()> cb) { m_onDragEnd = std::move(cb); }
bool VerticalSlider::dragging() const noexcept { return m_inputArea != nullptr && m_inputArea->pressed(); }

void VerticalSlider::doLayout(Renderer&) {
  updateGeometry();
  applyVisualState();
}

LayoutSize VerticalSlider::doMeasure(Renderer& renderer, const LayoutConstraints& constraints) {
  return measureByLayout(renderer, constraints);
}

void VerticalSlider::doArrange(Renderer& renderer, const LayoutRect& rect) {
  arrangeByLayout(renderer, rect);
}

void VerticalSlider::updateGeometry() {
  const float heightPx = height() > 0.0F ? height() : Style::sliderDefaultWidth;
  const float widthPx = std::max({m_thumbSizePx, m_trackWidth, m_controlWidthPx});
  setSize(widthPx, heightPx);

  const float trackX = (widthPx - m_trackWidth) * 0.5F;
  const float trackY = Style::sliderHorizontalPadding;
  const float trackH = std::max(0.0F, heightPx - Style::sliderHorizontalPadding * 2.0F);
  const float t = normalizedValue();
  // Value increases upward: thumb at bottom when t=0, top when t=1
  const float thumbY = trackY + (1.0F - t) * trackH;
  const float thumbX = (widthPx - m_thumbSizePx) * 0.5F;

  m_track->setPosition(trackX, trackY);
  m_track->setFrameSize(m_trackWidth, trackH);

  // Fill goes from thumb to bottom
  const float fillY = thumbY;
  const float fillHeight = trackY + trackH - thumbY;
  m_fill->setPosition(trackX, fillY);
  m_fill->setFrameSize(m_trackWidth, std::max(0.0F, fillHeight));

  m_thumb->setPosition(
      thumbX,
      util::clampOrdered(thumbY - m_thumbSizePx * 0.5F, trackY, trackY + trackH - m_thumbSizePx)
  );
  m_thumb->setFrameSize(m_thumbSizePx, m_thumbSizePx);

  m_inputArea->setPosition(0.0F, 0.0F);
  m_inputArea->setFrameSize(widthPx, heightPx);
}

void VerticalSlider::updateFromLocalY(float y) {
  const float heightPx = height() > 0.0F ? height() : Style::sliderDefaultWidth;
  const float trackY = Style::sliderHorizontalPadding;
  const float trackH = std::max(0.0F, heightPx - Style::sliderHorizontalPadding * 2.0F);
  if (trackH <= 0.0F) return;
  // Top = max, bottom = min
  const double raw = static_cast<double>(std::clamp((y - trackY) / trackH, 0.0F, 1.0F));
  const double t = 1.0 - raw;
  setValue(m_min + t * (m_max - m_min));
}

void VerticalSlider::applyVisualState() {
  const bool hovering = m_inputArea != nullptr && m_inputArea->hovered();
  const bool pressing = m_inputArea != nullptr && m_inputArea->pressed();
  const bool focused = m_inputArea != nullptr && m_inputArea->focused();

  Color trackColor = resolved(ColorRole::Outline);
  Color fillColor = resolved(ColorRole::Primary);
  Color thumbColor = resolved(ColorRole::OnPrimary);
  Color thumbBorder = resolved(ColorRole::Outline);

  m_thumb->setVisible(m_enabled);

  if (!m_enabled) {
    trackColor = resolved(ColorRole::Outline, Style::disabledOutlineAlpha);
    fillColor = resolved(ColorRole::Primary, 0.5F);
  } else if (pressing) {
    fillColor = resolved(ColorRole::Primary);
  } else if (focused) {
    thumbBorder = resolveColorSpec(focusRingColorSpec());
  } else if (hovering) {
    thumbBorder = resolved(ColorRole::Hover);
  }

  auto trackStyle = solidStyle(trackColor, m_trackWidth * 0.5F);
  m_track->setStyle(trackStyle);

  auto fillStyle = solidStyle(fillColor, m_trackWidth * 0.5F);
  m_fill->setStyle(fillStyle);

  auto thumbStyle = solidStyle(thumbColor, m_thumbSizePx * 0.5F);
  thumbStyle.border = thumbBorder;
  thumbStyle.borderWidth = focused ? Style::focusRingWidth : Style::borderWidth;
  m_thumb->setStyle(thumbStyle);
}

float VerticalSlider::normalizedValue() const noexcept {
  if (m_max <= m_min) return 0.0F;
  return static_cast<float>(std::clamp((m_value - m_min) / (m_max - m_min), 0.0, 1.0));
}

double VerticalSlider::snapped(double value) const noexcept {
  const double clamped = std::clamp(value, m_min, m_max);
  if (m_step <= 0.0 || m_max <= m_min) return clamped;
  const double steps = std::round((clamped - m_min) / m_step);
  return std::clamp(m_min + steps * m_step, m_min, m_max);
}
