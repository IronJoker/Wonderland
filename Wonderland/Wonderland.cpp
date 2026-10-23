#include "Wonderland.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "WonderlandControls.h"
#endif

Wonderland::Wonderland(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  for (int i = 0; i < ambi::Granular::kMaxGrains; i++)
  {
    mViz.env[i].store(0.f);
    mViz.pan[i].store(0.5f);
    mViz.pitch[i].store(0.f);
  }

  GetParam(kGrainAmount)->InitPercentage("grains", 0., 0., 100., 0, "granular");
  GetParam(kGrainMode)->InitEnum("mode", kModeFree, { "free", "sync" }, 0, "granular");
  GetParam(kGrainDiv)->InitEnum("rate", 2, { "1/2", "1/4", "1/8", "1/16", "1/32" }, 0, "granular");
  GetParam(kGrainFeel)->InitEnum("feel", kFeelStraight, { "straight", "triplet", "dotted" }, 0, "granular");
  GetParam(kGrainSize)->InitDouble("length", 160., 10., 1000., 0.1, "ms", 0, "granular", IParam::ShapePowCurve(2.));
  GetParam(kGrainGate)->InitPercentage("gate", 50., 5., 100., 0, "granular");
  GetParam(kGrainChaos)->InitPercentage("chaos", 0., 0., 100., 0, "granular");
  // tune: 0.5 cent resolution, shown (and typed) with at most 2 decimals
  GetParam(kGrainPitch)->InitDouble("tune", 0., -12., 12., 0.005, "st", 0, "granular", IParam::ShapeLinear(), IParam::kUnitCustom,
    [](double v, WDL_String& str) { str.SetFormatted(32, "%.2f", std::fabs(v) < 0.005 ? 0.0 : v); });
  GetParam(kSpace)->InitPercentage("space", 0., 0., 100., 0, "space");
  GetParam(kWidth)->InitPercentage("width", 100., 0., 200., 0, "output");
  GetParam(kMix)->InitPercentage("mix", 100., 0., 100., 0, "output");
  GetParam(kOutGain)->InitDouble("out", 0., -24., 12., 0.1, "db", 0, "output");

  // Factory presets (all lowercase). Enums are passed as int, everything else as double.
  MakePresetFromNamedParams("default", 0);
  MakePresetFromNamedParams("stab", 6,
    kGrainMode, (int) kModeSync, kGrainDiv, 2, kGrainGate, 25., kGrainAmount, 100., kGrainChaos, 10., kSpace, 25.);
  MakePresetFromNamedParams("lush", 5,
    kGrainAmount, 60., kGrainSize, 300., kGrainChaos, 45., kSpace, 60., kWidth, 150.);
  MakePresetFromNamedParams("mellow", 6,
    kGrainAmount, 35., kGrainSize, 380., kGrainChaos, 15., kSpace, 45., kMix, 80., kWidth, 105.);
  MakePresetFromNamedParams("choppy granulizer", 6,
    kGrainMode, (int) kModeSync, kGrainDiv, 3, kGrainGate, 45., kGrainAmount, 100., kGrainChaos, 25., kSpace, 15.);
  MakePresetFromNamedParams("frozen lake", 5,
    kGrainAmount, 70., kGrainSize, 600., kGrainChaos, 70., kSpace, 85., kWidth, 170.);
  MakePresetFromNamedParams("aurora", 6,
    kGrainAmount, 60., kGrainPitch, 12., kGrainChaos, 70., kGrainSize, 320., kSpace, 70., kWidth, 170.);
  MakePresetFromNamedParams("snow globe", 5,
    kGrainAmount, 50., kGrainSize, 40., kGrainChaos, 85., kGrainPitch, 12., kSpace, 50.);
  MakePresetFromNamedParams("half-time haze", 7,
    kGrainMode, (int) kModeSync, kGrainDiv, 1, kGrainGate, 90., kGrainPitch, -12., kGrainAmount, 70., kGrainChaos, 15., kSpace, 55.);
  MakePresetFromNamedParams("triplet drift", 8,
    kGrainMode, (int) kModeSync, kGrainDiv, 2, kGrainFeel, (int) kFeelTriplet, kGrainGate, 70., kGrainAmount, 80.,
    kGrainChaos, 30., kSpace, 40., kWidth, 150.);
  MakePresetFromNamedParams("stutter frost", 6,
    kGrainMode, (int) kModeSync, kGrainDiv, 4, kGrainGate, 60., kGrainAmount, 100., kGrainChaos, 70., kSpace, 20.);
  MakePresetFromNamedParams("polar night", 5,
    kGrainAmount, 55., kGrainPitch, -12., kGrainSize, 450., kGrainChaos, 30., kSpace, 75.);
  MakePresetFromNamedParams("glass halo", 6,
    kGrainAmount, 55., kGrainPitch, 7., kGrainSize, 240., kGrainChaos, 40., kSpace, 65., kWidth, 160.);
  MakePresetFromNamedParams("dream sequence", 8,
    kGrainMode, (int) kModeSync, kGrainDiv, 1, kGrainFeel, (int) kFeelDotted, kGrainGate, 80., kGrainAmount, 75.,
    kGrainChaos, 30., kSpace, 65., kWidth, 160.);
  MakePresetFromNamedParams("subtle polish", 6,
    kGrainAmount, 15., kGrainSize, 140., kGrainChaos, 20., kSpace, 20., kWidth, 115., kMix, 60.);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    using namespace wui;

    if (pGraphics->NControls())
      return; // fixed layout; resizing zooms the whole UI

    pGraphics->SetScaleConstraints(0.5f, 3.f);
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false, COLOR_TRANSPARENT, kInkFaint.WithOpacity(0.5f), kInkSoft.WithOpacity(0.6f), 22.f);
    pGraphics->LoadFont("Poppins-ExtraLight", POPPINS_XL_FN);
    pGraphics->LoadFont("Poppins-Light", POPPINS_L_FN);
    pGraphics->LoadFont("Poppins-Regular", POPPINS_R_FN);
    pGraphics->LoadFont("Poppins-Medium", POPPINS_M_FN);
    pGraphics->EnableMouseOver(true);

    const IRECT b = pGraphics->GetBounds();
    const float cx = b.MW();

    // knob row: [space width] | [length GRAINS chaos] | [tune mix]
    const float rowY = 438.f;
    const float smallD = 84.f, bigD = 176.f;
    const float xs[] = { cx - 425.f, cx - 310.f, cx - 178.f, cx, cx + 178.f, cx + 310.f, cx + 425.f };

    pGraphics->AttachControl(new WBackground(b, "version " PLUG_VERSION_STR));

    auto knob = [&](float x, float d, int param, const char* label, IColor accent, float origin = 0.f) {
      const float half = std::max(58.f, d * 0.5f + 14.f);
      auto* k = new WKnob(IRECT(x - half, rowY - d * 0.5f - 12.f, x + half, rowY + d * 0.5f + 70.f), param, label, accent, d * 0.5f, origin);
      k->EnableTextEntry(true);
      pGraphics->AttachControl(k);
      return k;
    };

    knob(xs[0], smallD, kSpace, "space", kLavender);
    knob(xs[1], smallD, kWidth, "width", kAzure, 0.5f);
    knob(xs[2], smallD, kGrainSize, "length", kAzure)->SetVisibleWhen(kGrainMode, kModeFree);
    knob(xs[2], smallD, kGrainGate, "gate", kAzure)->SetVisibleWhen(kGrainMode, kModeSync);
    knob(xs[3], bigD, kGrainAmount, "grains", kAzure);
    knob(xs[4], smallD, kGrainChaos, "chaos", kLavender);
    knob(xs[5], smallD, kGrainPitch, "tune", kTeal, 0.5f);
    knob(xs[6], smallD, kMix, "mix", kTeal);

    // header: title (left), visualizer (centre), preset + out (right)
    pGraphics->AttachControl(new WPresetPill(IRECT(b.R - 270.f, 24.f, b.R - 36.f, 90.f)));
    pGraphics->AttachControl(new WPlasma(IRECT(cx - 200.f, 26.f, cx + 200.f, 150.f), &mViz,
      [this](WDL_String& str) {
        if (GetParam(kGrainMode)->Int() == kModeSync)
        {
          static const char* feel[] = { "", "t", "d" };
          str.SetFormatted(64, "sync  %s%s   %.0f bpm", GetParam(kGrainDiv)->GetDisplayText(GetParam(kGrainDiv)->Int()),
                           feel[GetParam(kGrainFeel)->Int()], mViz.bpm.load());
        }
        else
          str.SetFormatted(64, "free   %.0f ms", GetParam(kGrainSize)->Value());
      }));
    auto* pOut = new WKnob(IRECT(b.R - 213.f, 112.f, b.R - 93.f, 262.f), kOutGain, "out", kInkSoft, 30.f, 24.f / 36.f);
    pOut->EnableTextEntry(true);
    pGraphics->AttachControl(pOut);

    // mode + sync rate/feel
    pGraphics->AttachControl(new WSegment(IRECT(cx - 100.f, 178.f, cx + 100.f, 214.f), kGrainMode, { "free", "sync" }));
    auto* pDiv = new WSegment(IRECT(cx - 200.f, 228.f, cx + 200.f, 262.f), kGrainDiv, { "1/2", "1/4", "1/8", "1/16", "1/32" });
    pDiv->SetActiveWhen(kGrainMode, kModeSync);
    pGraphics->AttachControl(pDiv);
    auto* pFeel = new WSegment(IRECT(cx - 160.f, 272.f, cx + 160.f, 304.f), kGrainFeel, { "straight", "triplet", "dotted" });
    pFeel->SetActiveWhen(kGrainMode, kModeSync);
    pGraphics->AttachControl(pFeel);

#if defined(_DEBUG) && defined(APP_API)
    pGraphics->AttachControl(new WSnapshot(b));
#endif
  };
#endif
}

#if IPLUG_EDITOR
// Hosts report the new window size after a resize. Treat it as a zoom of the fixed
// layout (keeping the aspect ratio) instead of a bigger canvas at 1x.
void Wonderland::OnParentWindowResize(int width, int height)
{
  if (auto* pGraphics = GetUI())
  {
    const float ps = pGraphics->GetPlatformWindowScale();
    const float s = std::clamp(std::min(width / ps / (float) PLUG_WIDTH, height / ps / (float) PLUG_HEIGHT), 0.5f, 3.f);
    pGraphics->Resize(PLUG_WIDTH, PLUG_HEIGHT, s, false);
  }
}

bool Wonderland::ConstrainEditorResize(int& w, int& h) const
{
  const float s = std::clamp(std::min(w / (float) PLUG_WIDTH, h / (float) PLUG_HEIGHT), 0.5f, 3.f);
  const int nw = (int) std::lround(PLUG_WIDTH * s), nh = (int) std::lround(PLUG_HEIGHT * s);
  const bool unchanged = (nw == w && nh == h);
  w = nw;
  h = nh;
  return unchanged;
}
#endif

#if defined CLAP_API
bool Wonderland::stateSave(const clap_ostream* pStream) noexcept
{
  IByteChunk chunk;
  if (!SerializeState(chunk))
    return false;

  const uint8_t* pData = chunk.GetData();
  int64_t remaining = chunk.Size();
  while (remaining > 0)
  {
    const int64_t written = pStream->write(pStream, pData, static_cast<uint64_t>(remaining));
    if (written <= 0)
      return false;
    pData += written;
    remaining -= written;
  }
  return true;
}

bool Wonderland::paramsValueToText(clap_id paramIdx, double value, char* display, uint32_t size) noexcept
{
  const IParam* pParam = GetParam(paramIdx);
  if (!pParam)
    return false;
  const bool isDoubleType = pParam->Type() == IParam::kTypeDouble;
  if (!isDoubleType)
    value = std::round(value);

  WDL_String str;
  pParam->GetDisplay(value, isDoubleType, str);
  if (CStringHasContents(pParam->GetLabel()))
  {
    str.Append(" ");
    str.Append(pParam->GetLabel());
  }
  if (size <= (uint32_t) str.GetLength())
    return false;
  strcpy(display, str.Get());
  return true;
}
#endif

#if IPLUG_DSP
void Wonderland::OnReset()
{
  const double sr = GetSampleRate();
  mGranular.Prepare(sr);
  mPlate.Prepare(sr);
  mWidener.Prepare(sr);

  ambi::Smoother* smoothers[] = { &mGrainMix, &mPlateMix, &mWidthS, &mDryWet, &mOutGainS };
  for (auto* s : smoothers) s->Prepare(sr, 30.0);

  UpdateDSPParams();
  mGrainMix.Reset(GetParam(kGrainAmount)->Value() * 0.01);
  mPlateMix.Reset(GetParam(kSpace)->Value() * 0.01 * 0.7);
  mWidthS.Reset(GetParam(kWidth)->Value() * 0.01);
  mDryWet.Reset(GetParam(kMix)->Value() * 0.01);
  mOutGainS.Reset(DBToAmp(GetParam(kOutGain)->Value()));

  mFreeStepPhase = 0.0;
  mLastStepIdx = -1;
  SetTailSize((int) (sr * 10.0));
}

// Length of one sync step in quarter-note beats
double Wonderland::StepBeats() const
{
  const int div = kDivDenominators[std::clamp(GetParam(kGrainDiv)->Int(), 0, kNumDivs - 1)];
  double beats = 4.0 / div;
  switch (GetParam(kGrainFeel)->Int())
  {
    case kFeelTriplet: beats *= 2.0 / 3.0; break;
    case kFeelDotted:  beats *= 1.5; break;
    default: break;
  }
  return beats;
}

void Wonderland::UpdateDSPParams()
{
  const double sr = GetSampleRate();
  double bpm = GetTempo();
  if (!(bpm > 1.0)) bpm = 120.0;

  mGranular.SetCommon(GetParam(kGrainPitch)->Value(), GetParam(kGrainChaos)->Value() * 0.01);
  if (GetParam(kGrainMode)->Int() == kModeSync)
    mGranular.SetSync(StepBeats() * 60.0 / bpm * sr, GetParam(kGrainGate)->Value() * 0.01);
  else
    mGranular.SetFree(GetParam(kGrainSize)->Value());

  // one "space" knob: bigger plate and more of it
  const double space = GetParam(kSpace)->Value() * 0.01;
  mPlate.SetParams(0.35 + 0.6 * space, 0.35, 15.0);

  mGrainMix.SetTarget(GetParam(kGrainAmount)->Value() * 0.01);
  mPlateMix.SetTarget(space * 0.7);
  mWidthS.SetTarget(GetParam(kWidth)->Value() * 0.01);
  mDryWet.SetTarget(GetParam(kMix)->Value() * 0.01);
  mOutGainS.SetTarget(DBToAmp(GetParam(kOutGain)->Value()));
}

void Wonderland::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  UpdateDSPParams();

  const int nIn = NInChansConnected();
  const int nOut = NOutChansConnected();
  const double halfPi = 0.5 * ambi::kPi;
  const double sr = GetSampleRate();

  // tempo sync: follow the host's beat position while playing, an internal clock otherwise
  const bool sync = GetParam(kGrainMode)->Int() == kModeSync;
  double bpm = GetTempo();
  if (!(bpm > 1.0)) bpm = 120.0;
  const double stepBeats = StepBeats();
  const double beatsPerSample = bpm / (60.0 * sr);
  const bool playing = GetTransportIsRunning() && GetPPQPos() >= 0.0;
  const double ppq0 = GetPPQPos();
  if (playing != mWasPlaying) { mLastStepIdx = -1; mWasPlaying = playing; }

  double sumSq = 0.0;
  double stepPhase = 0.0;

  for (int s = 0; s < nFrames; s++)
  {
    if (sync)
    {
      if (playing)
      {
        const double ppq = ppq0 + s * beatsPerSample;
        const double stepPos = ppq / stepBeats + 1e-9;
        const long long idx = (long long) std::floor(stepPos);
        stepPhase = stepPos - idx;
        if (idx != mLastStepIdx) { mLastStepIdx = idx; mGranular.Trigger(); }
      }
      else
      {
        mFreeStepPhase += beatsPerSample / stepBeats;
        if (mFreeStepPhase >= 1.0 || mLastStepIdx < 0)
        {
          mFreeStepPhase -= std::floor(mFreeStepPhase);
          mLastStepIdx = 0;
          mGranular.Trigger();
        }
        stepPhase = mFreeStepPhase;
      }
    }

    const double dryL = nIn > 0 ? inputs[0][s] : 0.0;
    const double dryR = nIn > 1 ? inputs[1][s] : dryL;
    double l = dryL, r = dryR, wl, wr;

    // granulizer (equal-power blend)
    const double g = mGrainMix.Process();
    mGranular.Process(l, r, wl, wr);
    {
      const double a = std::cos(g * halfPi), b = std::sin(g * halfPi);
      l = l * a + wl * b;
      r = r * a + wr * b;

    }

    // plate reverb (equal-power blend)
    const double p = mPlateMix.Process();
    mPlate.Process(l, r, wl, wr);
    {
      const double a = std::cos(p * halfPi), b = std::sin(p * halfPi);
      l = l * a + wl * b;
      r = r * a + wr * b;
    }

    // stereo width
    mWidener.Process(mWidthS.Process(), l, r);

    // global dry/wet + output gain
    const double m = mDryWet.Process();
    const double gain = mOutGainS.Process();
    l = (dryL + m * (l - dryL)) * gain;
    r = (dryR + m * (r - dryR)) * gain;

    if (nOut > 0) outputs[0][s] = l;
    if (nOut > 1) outputs[1][s] = r;
    sumSq += l * l + r * r;
  }

  // visualizer telemetry (lock-free, relaxed)
  const float rms = (float) std::sqrt(sumSq / std::max(1, 2 * nFrames));
  mLevel = std::max(rms, mLevel * 0.85f);
  mGranular.Snapshot(mVizEnv, mVizPan, mVizPitch);
  for (int i = 0; i < ambi::Granular::kMaxGrains; i++)
  {
    mViz.env[i].store(mVizEnv[i], std::memory_order_relaxed);
    mViz.pan[i].store(mVizPan[i], std::memory_order_relaxed);
    mViz.pitch[i].store(mVizPitch[i], std::memory_order_relaxed);
  }
  mViz.level.store(mLevel, std::memory_order_relaxed);
  mViz.stepPhase.store(sync ? (float) stepPhase : 0.f, std::memory_order_relaxed);
  mViz.triggers.store(mGranular.TriggerCount(), std::memory_order_relaxed);
  mViz.bpm.store((float) bpm, std::memory_order_relaxed);
}
#endif
