#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "WonderlandDSP.h"
#include <atomic>

const int kNumPresets = 15;

enum EParams
{
  kGrainAmount = 0, // "grains": how much granulizer you hear
  kGrainMode,       // free / sync
  kGrainDiv,        // sync: note division
  kGrainFeel,       // sync: straight / triplet / dotted
  kGrainSize,       // free: grain length (ms)
  kGrainGate,       // sync: how much of each step the stab lasts (%)
  kGrainChaos,
  kGrainPitch,      // "tune"
  kSpace,           // plate reverb amount (size + mix)
  kWidth,
  kMix,
  kOutGain,

  kNumParams
};

enum EGrainMode { kModeFree = 0, kModeSync };
enum EGrainFeel { kFeelStraight = 0, kFeelTriplet, kFeelDotted };

// note divisions offered in sync mode, as fractions of a whole note
inline constexpr int kNumDivs = 5;
inline constexpr int kDivDenominators[kNumDivs] = { 2, 4, 8, 16, 32 };

using namespace iplug;
using namespace igraphics;

// Lock-free data shared from the audio thread to the visualizer
struct GrainViz
{
  std::atomic<float> env[ambi::Granular::kMaxGrains];
  std::atomic<float> pan[ambi::Granular::kMaxGrains];
  std::atomic<float> pitch[ambi::Granular::kMaxGrains];
  std::atomic<float> level { 0.f };     // wet output level
  std::atomic<float> stepPhase { 0.f }; // 0..1 position inside the current sync step
  std::atomic<uint32_t> triggers { 0 };
  std::atomic<float> bpm { 120.f };
};

class Wonderland final : public Plugin
{
public:
  Wonderland(const InstanceInfo& info);

#if IPLUG_EDITOR
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override { return true; }
  void OnParentWindowResize(int width, int height) override;
  bool ConstrainEditorResize(int& w, int& h) const override;
#endif

  GrainViz mViz;

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;

private:
  void UpdateDSPParams();
  double StepBeats() const;

  ambi::Granular mGranular;
  ambi::PlateReverb mPlate;
  ambi::Widener mWidener;

  ambi::Smoother mGrainMix, mPlateMix, mWidthS, mDryWet, mOutGainS;
  double mSpaceDecay = 0.5;

  double mFreeStepPhase = 0.0;
  long long mLastStepIdx = -1;
  bool mWasPlaying = false;
  float mLevel = 0.f;
  float mVizEnv[ambi::Granular::kMaxGrains] = {};
  float mVizPan[ambi::Granular::kMaxGrains] = {};
  float mVizPitch[ambi::Granular::kMaxGrains] = {};
#endif
};
