#pragma once

// wonderland UI: flat, 2D, icy and minimal.
// Rules: every string is lowercase, everything is Poppins, no drop shadows, no gradients
// on controls. Solid fills and thin lines only.

#include "IControl.h"
#include "IPlugPluginBase.h"
#include "IGraphicsPopupMenu.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

using namespace iplug;
using namespace igraphics;

namespace wui
{

// ---- palette (flat) --------------------------------------------------------
inline const IColor kBg         { 255, 237, 241, 248 };
inline const IColor kInk        { 255,  24,  34,  60 };
inline const IColor kInkSoft    { 255,  82,  95, 126 };
inline const IColor kInkFaint   { 255, 150, 160, 184 };
inline const IColor kLine       { 255, 206, 214, 228 };
inline const IColor kTrack      { 255, 214, 221, 234 };
inline const IColor kNavy       { 255,  27,  38,  68 };
inline const IColor kNavyHover  { 255,  40,  54,  92 };
inline const IColor kScreen     { 255,  20,  30,  58 };
inline const IColor kDisabled   { 255, 186, 194, 210 };
inline const IColor kAzure      { 255,  74, 164, 240 };
inline const IColor kTeal       { 255,  44, 190, 176 };
inline const IColor kLavender   { 255, 142, 124, 232 };
inline const IColor kAmber      { 255, 246, 170,  96 };

inline const char* kFontXL = "Poppins-ExtraLight";
inline const char* kFontL  = "Poppins-Light";
inline const char* kFontR  = "Poppins-Regular";
inline const char* kFontM  = "Poppins-Medium";

inline IColor Mix(const IColor& a, const IColor& b, float t)
{
  auto l = [t](int x, int y) { return (int) std::lround(x + (y - x) * t); };
  return IColor(l(a.A, b.A), l(a.R, b.R), l(a.G, b.G), l(a.B, b.B));
}

inline std::string Lower(const char* s)
{
  std::string r(s ? s : "");
  for (auto& c : r) c = (char) std::tolower((unsigned char) c);
  return r;
}

inline bool Ease(float& v, float target, float speed)
{
  const float d = target - v;
  if (std::fabs(d) < 0.002f) { if (v != target) { v = target; return true; } return false; }
  v += d * speed;
  return true;
}

inline float TrackedWidth(IGraphics& g, const IText& text, const char* str, float tracking)
{
  const IText t = text.WithAlign(EAlign::Near);
  float total = 0.f;
  char buf[2] = { 0, 0 };
  int n = 0;
  for (const char* p = str; *p; ++p, ++n)
  {
    buf[0] = *p;
    IRECT r;
    total += g.MeasureText(t, buf, r);
  }
  return total + tracking * std::max(0, n - 1);
}

// Text with letter spacing. align: -1 left, 0 centre, 1 right. ascii only.
inline void DrawTracked(IGraphics& g, const IText& text, const char* str, float x, float y, float tracking, int align = 0)
{
  const IText t = text.WithAlign(EAlign::Near);
  const float total = TrackedWidth(g, text, str, tracking);
  float px = align < 0 ? x : (align == 0 ? x - total * 0.5f : x - total);
  char buf[2] = { 0, 0 };
  for (const char* p = str; *p; ++p)
  {
    buf[0] = *p;
    IRECT r;
    const float w = g.MeasureText(t, buf, r);
    g.DrawText(t, buf, IRECT(px, y - text.mSize, px + w + 2.f, y + text.mSize));
    px += w + tracking;
  }
}

inline void StrokeArc(IGraphics& g, const IColor& c, float cx, float cy, float r, float a1, float a2, float w)
{
  if (std::fabs(a2 - a1) < 0.01f) return;
  if (a2 < a1) std::swap(a1, a2);
  IStrokeOptions o;
  o.mCapOption = ELineCap::Round;
  g.PathClear();
  g.PathArc(cx, cy, r, a1, a2);
  g.PathStroke(IPattern(c), w, o);
}

// Flat four-point sparkle
inline void Sparkle(IGraphics& g, float cx, float cy, float r, const IColor& c)
{
  const float k = r * 0.2f;
  g.PathClear();
  g.PathMoveTo(cx, cy - r);
  g.PathLineTo(cx + k, cy - k);
  g.PathLineTo(cx + r, cy);
  g.PathLineTo(cx + k, cy + k);
  g.PathLineTo(cx, cy + r);
  g.PathLineTo(cx - k, cy + k);
  g.PathLineTo(cx - r, cy);
  g.PathLineTo(cx - k, cy - k);
  g.PathClose();
  g.PathFill(IPattern(c));
}

inline void SplitDisplay(const IParam* pParam, std::string& number, std::string& unit)
{
  number.clear(); unit.clear();
  if (!pParam) return;
  WDL_String s;
  pParam->GetDisplay(s);
  number = Lower(s.Get());
  unit = Lower(pParam->GetLabel());
  const auto sp = number.find(' ');
  if (sp != std::string::npos) { unit = number.substr(sp + 1); number = number.substr(0, sp); }
}

// Shared enable/visibility logic, based on another param's value
class Activity
{
public:
  void SetActiveWhen(int idx, int value) { mWhen = idx; mWhenValue = value; }
  void SetVisibleWhen(int idx, int value) { mShowWhen = idx; mShowValue = value; }

protected:
  bool IsActive(IEditorDelegate* pDelegate) const
  {
    return !pDelegate || mWhen == kNoParameter || pDelegate->GetParam(mWhen)->Int() == mWhenValue;
  }
  // returns true if visibility changed
  bool UpdateVisibility(IControl* pControl)
  {
    if (mShowWhen == kNoParameter || !pControl->GetDelegate()) return false;
    const bool show = pControl->GetDelegate()->GetParam(mShowWhen)->Int() == mShowValue;
    if (show == !pControl->IsHidden()) return false;
    pControl->Hide(!show);
    return true;
  }

private:
  int mWhen = kNoParameter, mWhenValue = 0;
  int mShowWhen = kNoParameter, mShowValue = 0;
};

// ---------------------------------------------------------------------------
// Flat knob: navy disc, white pointer, coloured arc ring, label + value below
// ---------------------------------------------------------------------------
class WKnob : public IKnobControlBase, public Activity
{
public:
  WKnob(const IRECT& bounds, int paramIdx, const char* label, IColor accent, float radius, float arcOrigin = 0.f)
  : IKnobControlBase(bounds, paramIdx, EDirection::Vertical, 1.0)
  , mLabel(label), mAccent(accent), mRadius(radius), mArcOrigin(arcOrigin)
  {}

  IRECT GetKnobDragBounds() override { return mRECT.GetCentredInside(10.f, 240.f); }

  bool IsDirty() override
  {
    bool m = UpdateVisibility(this);
    m |= Ease(mHover, (mMouseIsOver || mMouseDown) ? 1.f : 0.f, 0.25f);
    m |= Ease(mPress, mMouseDown ? 1.f : 0.f, 0.35f);
    m |= Ease(mActive, IsActive(GetDelegate()) ? 1.f : 0.f, 0.2f);
    return m || IKnobControlBase::IsDirty();
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override { GetUI()->SetMouseCursor(ECursor::HAND); IKnobControlBase::OnMouseOver(x, y, mod); }
  void OnMouseOut() override { GetUI()->SetMouseCursor(); IKnobControlBase::OnMouseOut(); }

  // allow typing a value by right/double-clicking the value text
  void EnableTextEntry(bool enable) { mTextEntry = enable; }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R)
    {
      if (mTextEntry && ValueRect().Contains(x, y)) StartTextEntry();
      else { SetValueToDefault(); SetDirty(true); }
      return;
    }
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
    if (mTextEntry && ValueRect().Contains(x, y)) { StartTextEntry(); return; }
    SetValueToDefault();
    SetDirty(true);
  }

  void OnTextEntryCompletion(const char* str, int valIdx) override
  {
    if (!str || !GetParam()) return;
    std::string t(str);
    for (auto& c : t) if (c == ',') c = '.';
    char* end = nullptr;
    double v = std::strtod(t.c_str(), &end);
    if (end == t.c_str()) return; // not a number
    v = std::round(v * 100.0) / 100.0; // two decimals at most
    v = std::clamp(v, GetParam()->GetMin(), GetParam()->GetMax());
    SetValue(GetParam()->ToNormalized(v));
    SetDirty(true);
  }

  void Draw(IGraphics& g) override
  {
    const bool big = mRadius > 60.f;
    const float R = mRadius * (1.f - 0.03f * mPress);
    const float cx = mRECT.MW();
    const float cy = mRECT.T + 12.f + mRadius;

    // arc ring
    const float aMin = -140.f, aMax = 140.f;
    const float a = aMin + (float) GetValue() * (aMax - aMin);
    const float a0 = aMin + mArcOrigin * (aMax - aMin);
    const float ringR = mRadius + (big ? 9.f : 7.f);
    const float ringW = big ? 5.f : 3.5f;
    StrokeArc(g, kTrack, cx, cy, ringR, aMin, aMax, ringW);
    StrokeArc(g, Mix(kDisabled, mAccent, mActive), cx, cy, ringR, a0, a, ringW);

    // disc + pointer
    g.FillCircle(Mix(kDisabled, Mix(kNavy, kNavyHover, mHover), mActive), cx, cy, R);
    IStrokeOptions o;
    o.mCapOption = ELineCap::Round;
    g.PathClear();
    g.PathRadialLine(cx, cy, a, R * 0.42f, R * 0.82f);
    g.PathStroke(IPattern(COLOR_WHITE), big ? 4.f : 3.f, o);

    // label + value
    const float labelY = cy + mRadius + (big ? 34.f : 28.f);
    const IText lab(big ? 19.f : 16.f, Mix(kInkFaint, kInk, mActive), kFontR);
    g.DrawText(lab, mLabel.c_str(), IRECT(mRECT.L, labelY - 13.f, mRECT.R, labelY + 13.f));

    std::string num, unit;
    SplitDisplay(GetParam(), num, unit);
    const std::string val = unit.empty() ? num : num + " " + unit;
    const IText vt(big ? 17.f : 15.f, Mix(kInkFaint, Mix(kInkSoft, kInk, mHover), mActive), kFontL);
    g.DrawText(vt, val.c_str(), IRECT(mRECT.L - 6.f, labelY + 11.f, mRECT.R + 6.f, labelY + 37.f));
  }

private:
  float LabelY() const { return mRECT.T + 12.f + mRadius + mRadius + (mRadius > 60.f ? 34.f : 28.f); }
  IRECT ValueRect() const { const float y = LabelY(); return IRECT(mRECT.L, y + 9.f, mRECT.R, y + 39.f); }

  void StartTextEntry()
  {
    std::string num, unit;
    SplitDisplay(GetParam(), num, unit);
    const IText te = IText(15.f, kInk, kFontR).WithTEColors(COLOR_WHITE, kInk);
    GetUI()->CreateTextEntry(*this, te, ValueRect().GetCentredInside(80.f, 26.f), num.c_str());
  }

  std::string mLabel;
  IColor mAccent;
  float mRadius, mArcOrigin;
  bool mTextEntry = false;
  float mHover = 0.f, mPress = 0.f, mActive = 1.f;
};

// ---------------------------------------------------------------------------
// Flat segmented buttons (free / sync, rates, feel)
// ---------------------------------------------------------------------------
class WSegment : public IControl, public Activity
{
public:
  WSegment(const IRECT& bounds, int paramIdx, std::initializer_list<const char*> labels)
  : IControl(bounds, paramIdx)
  {
    for (auto* l : labels) mLabels.emplace_back(l);
    mHover.assign(mLabels.size(), 0.f);
  }

  bool IsDirty() override
  {
    bool m = UpdateVisibility(this);
    for (size_t i = 0; i < mLabels.size(); i++)
      m |= Ease(mHover[i], (mMouseIsOver && (int) i == mHoverIdx) ? 1.f : 0.f, 0.3f);
    m |= Ease(mActive, IsActive(GetDelegate()) ? 1.f : 0.f, 0.2f);
    m |= Ease(mSel, (float) Selected(), 0.35f);
    return m || IControl::IsDirty();
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->SetMouseCursor(ECursor::HAND);
    mHoverIdx = IndexAt(x);
    IControl::OnMouseOver(x, y, mod);
    SetDirty(false);
  }
  void OnMouseOut() override { GetUI()->SetMouseCursor(); mHoverIdx = -1; IControl::OnMouseOut(); }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R) { SetValueToDefault(); SetDirty(true); return; }
    const int idx = IndexAt(x);
    if (idx < 0) return;
    SetValue(mLabels.size() > 1 ? idx / (double) (mLabels.size() - 1) : 0.0);
    SetDirty(true);
  }

  void Draw(IGraphics& g) override
  {
    const IRECT r = mRECT;
    const float rad = r.H() * 0.5f;
    const int n = (int) mLabels.size();
    const float segW = r.W() / n;

    g.FillRoundRect(Mix(kBg, kTrack, 0.8f), r, rad);

    const IRECT sel(r.L + mSel * segW + 3.f, r.T + 3.f, r.L + (mSel + 1.f) * segW - 3.f, r.B - 3.f);
    g.FillRoundRect(Mix(kDisabled, kNavy, mActive), sel, sel.H() * 0.5f);

    const int selected = Selected();
    for (int i = 0; i < n; i++)
    {
      const IRECT cell(r.L + i * segW, r.T, r.L + (i + 1) * segW, r.B);
      const bool isSel = i == selected;
      const IColor c = isSel ? COLOR_WHITE : Mix(kInkFaint, Mix(kInkSoft, kInk, mHover[i]), mActive);
      g.DrawText(IText(std::min(15.f, r.H() * 0.46f), c, isSel ? kFontM : kFontR), mLabels[i].c_str(), cell.GetVShifted(0.5f));
    }
  }

private:
  int Selected() const
  {
    const int n = (int) mLabels.size();
    return std::clamp((int) std::lround(GetValue() * (n - 1)), 0, n - 1);
  }
  int IndexAt(float x) const
  {
    const int n = (int) mLabels.size();
    const int i = (int) ((x - mRECT.L) / (mRECT.W() / n));
    return (i >= 0 && i < n) ? i : -1;
  }

  std::vector<std::string> mLabels;
  std::vector<float> mHover;
  int mHoverIdx = -1;
  float mActive = 1.f, mSel = 0.f;
};

// ---------------------------------------------------------------------------
// Plasma visualizer (flat): wave lines that swell with the granulizer, one dot per
// live grain (x = stereo position, height + colour = pitch)
// ---------------------------------------------------------------------------
template <class VIZ>
class WPlasmaT : public IControl
{
public:
  using InfoFunc = std::function<void(WDL_String&)>;

  WPlasmaT(const IRECT& bounds, const VIZ* pViz, InfoFunc info)
  : IControl(bounds), mViz(pViz), mInfo(std::move(info))
  {
    mIgnoreMouse = true;
    mStart = std::chrono::steady_clock::now();
  }

  bool IsDirty() override { return true; } // animated

  void Draw(IGraphics& g) override
  {
    const IRECT r = mRECT;
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::clamp(std::chrono::duration<float>(now - mStart).count(), 0.f, 0.1f);
    mStart = now;

    const float level = mViz ? mViz->level.load(std::memory_order_relaxed) : 0.f;
    mEnergy += (std::min(1.f, level * 4.f) - mEnergy) * (level * 4.f > mEnergy ? 0.35f : 0.06f);
    const bool sounding = mEnergy > 0.02f;
    const uint32_t trig = mViz ? mViz->triggers.load(std::memory_order_relaxed) : 0;
    if (trig != mLastTrig) { if (sounding) mPulse = 1.f; mLastTrig = trig; }
    mPulse *= 0.82f;

    // idle: slow, gentle drift. playing: speeds up and swells with the sound
    mT += dt * (0.18f + 1.1f * mEnergy);
    const float t = mT;

    g.FillRoundRect(kScreen, r, 16.f);

    float env[64] = {}, pan[64] = {}, pitch[64] = {};
    const int nG = mViz ? std::min(64, (int) (sizeof(mViz->env) / sizeof(mViz->env[0]))) : 0;
    for (int i = 0; i < nG; i++)
    {
      env[i] = mViz->env[i].load(std::memory_order_relaxed);
      pan[i] = mViz->pan[i].load(std::memory_order_relaxed);
      pitch[i] = mViz->pitch[i].load(std::memory_order_relaxed);
    }

    struct Wave { float freq, speed, phase, amp; IColor c; };
    const Wave waves[] = {
      { 1.6f, 1.1f, 0.0f, 1.00f, kAzure },
      { 2.3f, -0.8f, 1.7f, 0.80f, kLavender },
      { 3.1f, 1.5f, 3.1f, 0.55f, kTeal },
    };

    const float pad = 18.f;
    const float x0 = r.L + pad, x1 = r.R - pad;
    const float cy = r.MH() + 8.f;
    const float baseAmp = r.H() * (0.05f + 0.25f * mEnergy + 0.10f * mPulse);
    const float grainVis = std::min(1.f, mEnergy * 5.f);

    IStrokeOptions o;
    o.mCapOption = ELineCap::Round;
    o.mJoinOption = ELineJoin::Round;
    for (const auto& w : waves)
    {
      g.PathClear();
      bool first = true;
      for (float x = x0; x <= x1; x += 3.f)
      {
        const float u = (x - x0) / (x1 - x0);
        const float edge = std::sin(u * 3.14159265f);
        float bump = 0.f;
        for (int i = 0; i < nG; i++)
        {
          if (env[i] < 0.01f) continue;
          const float d = (u - pan[i]) * 6.f;
          bump += env[i] * std::exp(-d * d);
        }
        const float y = cy + edge * (baseAmp * w.amp * std::sin(u * w.freq * 6.2831853f + t * w.speed + w.phase)
                                     + r.H() * 0.12f * w.amp * grainVis * std::min(1.5f, bump) * std::sin(t * 2.5f + w.phase));
        if (first) { g.PathMoveTo(x, y); first = false; } else g.PathLineTo(x, y);
      }
      g.PathStroke(IPattern(w.c.WithOpacity(0.45f + 0.55f * std::min(1.f, mEnergy * 2.f))), 2.f, o);
    }

    for (int i = 0; i < nG; i++)
    {
      if (env[i] < 0.02f || grainVis < 0.02f) continue;
      const float x = x0 + pan[i] * (x1 - x0);
      const float pn = std::clamp((pitch[i] + 12.f) / 36.f, 0.f, 1.f);
      const float y = r.B - 20.f - pn * (r.H() - 46.f);
      const IColor c = pn < 0.5f ? Mix(kTeal, kAzure, pn * 2.f) : Mix(kLavender, kAmber, (pn - 0.5f) * 2.f);
      g.FillCircle(c.WithOpacity((0.35f + 0.65f * env[i]) * grainVis), x, y, 2.5f + 4.f * env[i]);
    }

    const float phase = mViz ? mViz->stepPhase.load(std::memory_order_relaxed) : 0.f;
    if (phase > 0.f && sounding)
    {
      g.DrawLine(COLOR_WHITE.WithOpacity(0.12f), x0, r.B - 10.f, x1, r.B - 10.f, nullptr, 2.f);
      g.DrawLine(kAzure, x0, r.B - 10.f, x0 + phase * (x1 - x0), r.B - 10.f, nullptr, 2.f);
    }

    WDL_String info;
    if (mInfo) mInfo(info);
    g.DrawText(IText(13.f, IColor(255, 190, 202, 228), kFontR, EAlign::Near), info.Get(), IRECT(r.L + 18.f, r.T + 8.f, r.R - 40.f, r.T + 30.f));
    Sparkle(g, r.R - 22.f, r.T + 19.f, 5.f + 2.f * mPulse, kAmber);
  }

private:
  const VIZ* mViz;
  InfoFunc mInfo;
  std::chrono::steady_clock::time_point mStart;
  uint32_t mLastTrig = 0;
  float mPulse = 0.f, mEnergy = 0.f, mT = 0.f;
};

// ---------------------------------------------------------------------------
// Background: flat icy colour, title, group dividers, footer
// ---------------------------------------------------------------------------
class WBackground : public IControl
{
public:
  WBackground(const IRECT& bounds, const char* footer)
  : IControl(bounds), mFooter(footer)
  { mIgnoreMouse = true; }

  void Draw(IGraphics& g) override
  {
    const IRECT b = mRECT;
    g.FillRect(kBg, b);

    // title (the logo star stays)
    Sparkle(g, b.L + 50.f, b.T + 52.f, 12.f, kAmber);
    // one weight step heavier for both halves (extralight/regular -> light/medium), same contrast
    const IText light(34.f, kInkSoft, kFontL), dark(34.f, kInk, kFontM);
    const float tx = b.L + 74.f, ty = b.T + 54.f, tr = 3.5f;
    const float w1 = TrackedWidth(g, light, "wonder", tr);
    DrawTracked(g, light, "wonder", tx, ty, tr, -1);
    DrawTracked(g, dark, "land", tx + w1 + tr, ty, tr, -1);
    DrawTracked(g, IText(14.f, kInkSoft, kFontR), "by simi soprano", tx + 2.f, ty + 34.f, 2.2f, -1);

    DrawTracked(g, IText(13.f, kInkSoft, kFontR), mFooter.c_str(), b.MW(), b.B - 20.f, 2.f, 0);
  }

private:
  std::string mFooter;
};

// ---------------------------------------------------------------------------
// Preset pill: flat navy, lowercase name, caret. Click = menu, scroll = prev/next
// ---------------------------------------------------------------------------
class WPresetPill : public IControl
{
public:
  explicit WPresetPill(const IRECT& bounds) : IControl(bounds) {}

  bool IsDirty() override
  {
    bool m = false;
    for (int i = 0; i < 3; i++) m |= Ease(mHover[i], (mMouseIsOver && i == mHoverZone) ? 1.f : 0.f, 0.3f);
    if (auto* p = Plug(); p && p->GetCurrentPresetIdx() != mLastIdx) { mLastIdx = p->GetCurrentPresetIdx(); m = true; }
    return m || IControl::IsDirty();
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->SetMouseCursor(ECursor::HAND);
    mHoverZone = Zone(x, y);
    IControl::OnMouseOver(x, y, mod);
    SetDirty(false);
  }
  void OnMouseOut() override { GetUI()->SetMouseCursor(); mHoverZone = -1; IControl::OnMouseOut(); }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    auto* p = Plug();
    if (!p || !p->NPresets()) return;
    const int n = p->NPresets();
    const int zone = Zone(x, y);
    if (mod.R) { Restore(0); return; } // right-click: back to "default"
    if (zone == 0) { Restore((p->GetCurrentPresetIdx() - 1 + n) % n); return; }
    if (zone == 2) { Restore((p->GetCurrentPresetIdx() + 1) % n); return; }
    if (zone != 1) return;
    mMenu.Clear();
    for (int i = 0; i < n; i++)
    {
      auto* item = mMenu.AddItem(Lower(p->GetPresetName(i)).c_str());
      if (i == p->GetCurrentPresetIdx()) item->SetChecked(true);
    }
    GetUI()->CreatePopupMenu(*this, mMenu, Pill());
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    auto* p = Plug();
    if (!p || !p->NPresets()) return;
    const int n = p->NPresets();
    Restore((p->GetCurrentPresetIdx() + (d > 0 ? -1 : 1) + n) % n);
  }

  void OnPopupMenuSelection(IPopupMenu* pMenu, int valIdx) override
  {
    if (pMenu && pMenu->GetChosenItemIdx() >= 0) Restore(pMenu->GetChosenItemIdx());
  }

  void Draw(IGraphics& g) override
  {
    const IRECT pill = Pill();
    const float h = pill.H();

    // caption with index, e.g. "preset   3 / 15"
    WDL_String cap;
    if (auto* p = Plug(); p && p->NPresets()) cap.SetFormatted(32, "preset   %d / %d", p->GetCurrentPresetIdx() + 1, p->NPresets());
    DrawTracked(g, IText(13.f, kInkSoft, kFontR), cap.Get(), pill.L + h * 0.5f, mRECT.T + 9.f, 1.2f, -1);

    // pill with two round arrow buttons inside
    g.FillRoundRect(kNavy, pill, h * 0.5f);
    const IRECT left = ArrowRect(true), right = ArrowRect(false);
    g.FillCircle(Mix(kNavy, kNavyHover, 0.6f + 0.4f * mHover[0]), left.MW(), left.MH(), left.H() * 0.5f);
    g.FillCircle(Mix(kNavy, kNavyHover, 0.6f + 0.4f * mHover[2]), right.MW(), right.MH(), right.H() * 0.5f);
    Chevron(g, left, true);
    Chevron(g, right, false);

    std::string name;
    if (auto* p = Plug(); p && p->NPresets()) name = Lower(p->GetPresetName(p->GetCurrentPresetIdx()));
    const IRECT nameR(left.R + 6.f, pill.T, right.L - 6.f, pill.B);
    g.DrawText(IText(16.f, Mix(IColor(255, 222, 230, 248), COLOR_WHITE, mHover[1]), kFontR), name.c_str(), nameR.GetVShifted(0.5f));
    g.DrawLine(kAzure.WithOpacity(0.25f + 0.75f * mHover[1]), nameR.MW() - 16.f, pill.B - 8.f, nameR.MW() + 16.f, pill.B - 8.f, nullptr, 2.f);
  }

private:
  IRECT Pill() const { return IRECT(mRECT.L, mRECT.T + 22.f, mRECT.R, mRECT.B); }
  IRECT ArrowRect(bool left) const
  {
    const IRECT p = Pill();
    const float d = p.H() - 10.f;
    return left ? IRECT(p.L + 5.f, p.T + 5.f, p.L + 5.f + d, p.B - 5.f) : IRECT(p.R - 5.f - d, p.T + 5.f, p.R - 5.f, p.B - 5.f);
  }
  int Zone(float x, float y) const
  {
    if (!Pill().Contains(x, y)) return -1;
    if (ArrowRect(true).Contains(x, y)) return 0;
    if (ArrowRect(false).Contains(x, y)) return 2;
    return 1;
  }
  static void Chevron(IGraphics& g, const IRECT& r, bool left)
  {
    const float cx = r.MW() + (left ? -1.f : 1.f), cy = r.MH(), s = 4.5f;
    IStrokeOptions o;
    o.mCapOption = ELineCap::Round;
    o.mJoinOption = ELineJoin::Round;
    g.PathClear();
    g.PathMoveTo(cx + (left ? s * 0.5f : -s * 0.5f), cy - s);
    g.PathLineTo(cx + (left ? -s * 0.5f : s * 0.5f), cy);
    g.PathLineTo(cx + (left ? s * 0.5f : -s * 0.5f), cy + s);
    g.PathStroke(IPattern(COLOR_WHITE), 2.f, o);
  }

  iplug::IPluginBase* Plug() { return dynamic_cast<iplug::IPluginBase*>(GetDelegate()); }
  void Restore(int idx) { if (auto* p = Plug()) { p->RestorePreset(idx); mLastIdx = -1; SetDirty(false); } }

  IPopupMenu mMenu;
  int mLastIdx = -1, mHoverZone = -1;
  float mHover[3] = {};
};

#if defined(_DEBUG) && defined(APP_API)
// Debug-only: if WONDERLAND_SNAPSHOT=/path.rgba is set, renders the whole UI offscreen once
// and writes "width height\n" + raw RGBA. Works even when the screen is locked.
class WSnapshot : public IControl
{
public:
  explicit WSnapshot(const IRECT& bounds) : IControl(bounds) { mIgnoreMouse = true; }
  bool IsDirty() override { return !mDone && getenv("WONDERLAND_SNAPSHOT"); }
  void Draw(IGraphics& g) override
  {
    const char* path = getenv("WONDERLAND_SNAPSHOT");
    if (mDone || !path || ++mFrames < 30) return;
    mDone = true;
    g.StartLayer(this, mRECT);
    for (int i = 0; i < g.NControls(); i++)
    {
      IControl* c = g.GetControl(i);
      if (c != this && !c->IsHidden()) c->Draw(g);
    }
    ILayerPtr layer = g.EndLayer();
    RawBitmapData data;
    g.GetLayerBitmapData(layer, data);
    const APIBitmap* bmp = layer->GetAPIBitmap();
    if (FILE* f = fopen(path, "wb"))
    {
      fprintf(f, "%d %d\n", bmp->GetWidth(), bmp->GetHeight());
      fwrite(data.Get(), 1, data.GetSize(), f);
      fclose(f);
    }
  }
private:
  int mFrames = 0;
  bool mDone = false;
};
#endif

} // namespace wui

// The plasma visualizer is templated on the plugin's telemetry struct
using WPlasma = wui::WPlasmaT<GrainViz>;
