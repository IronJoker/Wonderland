#pragma once

// wonderland DSP building blocks.
// All memory is allocated in Prepare() (called from OnReset), never on the audio thread.

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace ambi
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline double Clamp(double x, double lo, double hi) { return x < lo ? lo : (x > hi ? hi : x); }

// Cheap deterministic RNG (xorshift32), returns [0, 1)
class Random
{
public:
  explicit Random(uint32_t seed = 0x9E3779B9u) : mState(seed ? seed : 1u) {}
  double Next()
  {
    mState ^= mState << 13;
    mState ^= mState >> 17;
    mState ^= mState << 5;
    return (mState >> 8) * (1.0 / 16777216.0);
  }
  double Bipolar() { return Next() * 2.0 - 1.0; }
private:
  uint32_t mState;
};

// One-pole parameter smoother
class Smoother
{
public:
  void Prepare(double sampleRate, double timeMs = 25.0)
  {
    mCoeff = 1.0 - std::exp(-1.0 / (0.001 * timeMs * sampleRate));
  }
  void Reset(double v) { mValue = mTarget = v; }
  void SetTarget(double v) { mTarget = v; }
  double Process() { mValue += mCoeff * (mTarget - mValue); return mValue; }
  double Value() const { return mValue; }
private:
  double mCoeff = 1.0, mValue = 0.0, mTarget = 0.0;
};

// Power-of-two circular delay line with fractional reads
class DelayLine
{
public:
  void Prepare(int maxSamples)
  {
    int size = 1;
    while (size < maxSamples + 4) size <<= 1;
    mBuf.assign(size, 0.f);
    mMask = size - 1;
    mWrite = 0;
  }
  void Clear() { std::fill(mBuf.begin(), mBuf.end(), 0.f); }
  void Write(double x) { mBuf[mWrite] = (float) x; mWrite = (mWrite + 1) & mMask; }
  // delay in samples, >= 1 means the most recently written sample is at delay 1
  double Read(int delay) const { return mBuf[(mWrite - delay) & mMask]; }
  double ReadLinear(double delay) const
  {
    const int i = (int) delay;
    const double f = delay - i;
    const double a = Read(i), b = Read(i + 1);
    return a + f * (b - a);
  }
  double ReadCubic(double delay) const
  {
    const int i = (int) delay;
    const double f = delay - i;
    const double y0 = Read(i - 1), y1 = Read(i), y2 = Read(i + 1), y3 = Read(i + 2);
    const double c1 = 0.5 * (y2 - y0);
    const double c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    const double c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + y1;
  }
private:
  std::vector<float> mBuf;
  int mMask = 0;
  int mWrite = 0;
};

// ---------------------------------------------------------------------------
// Granulizer: stereo record buffer + up to kMaxGrains grains. Grains read the live
// input (a few samples behind the record head), so there is no audible delay.
//  free mode: grains are spawned by an internal clock, sharp-attack window.
//  sync mode: the host clock calls Trigger() once per note division; each grain is a
//             "stab": ~1 ms attack, punchy decay, cut at the gate point.
//             chaos adds stutter repeats and octave jumps.
// ---------------------------------------------------------------------------
class Granular
{
public:
  static constexpr int kMaxGrains = 48;

  void Prepare(double sampleRate)
  {
    mSR = sampleRate;
    mBufLen = (int) (sampleRate * 8.0);
    mBufL.assign(mBufLen, 0.f);
    mBufR.assign(mBufLen, 0.f);
    mWrite = 0;
    mSamplesToNext = 0.0;
    mLastChopPos = -1.0;
    for (auto& g : mGrains) g.active = false;
  }

  // Shared settings. pitch in semitones, chaos 0..1
  void SetCommon(double pitch, double chaos)
  {
    mPitch = pitch; mChaos = chaos;
  }

  // free mode: grain length in ms (grains overlap about twice)
  void SetFree(double sizeMs)
  {
    mSync = false;
    mSizeSamples = sizeMs * 0.001 * mSR;
    mDensity = std::clamp(3.0 / (sizeMs * 0.001), 3.0, 120.0);
    mNorm = 0.6;
  }

  // sync mode: step length in samples (one note division at the host tempo), gate 0..1
  void SetSync(double stepSamples, double gate)
  {
    mSync = true;
    mSizeSamples = stepSamples;
    mGate = gate;
    mNorm = 1.0;
  }

  // sync mode: start a chop now
  void Trigger() { mPendingTrigger = true; }

  void Process(double inL, double inR, double& outL, double& outR)
  {
    mBufL[mWrite] = (float) inL;
    mBufR[mWrite] = (float) inR;

    if (mSync)
    {
      if (mPendingTrigger) { SpawnChop(); mPendingTrigger = false; }
    }
    else
    {
      mSamplesToNext -= 1.0;
      if (mSamplesToNext <= 0.0)
      {
        SpawnCloudGrain();
        const double interval = mSR / std::max(0.5, mDensity);
        mSamplesToNext += interval * (1.0 + mChaos * 0.9 * mRand.Bipolar());
        if (mSamplesToNext < 1.0) mSamplesToNext = 1.0;
      }
    }

    double l = 0.0, r = 0.0;
    for (auto& g : mGrains)
    {
      if (!g.active) continue;
      const double w = Window(g);
      g.env = (float) w;
      double sl, sr;
      ReadStereo(g.pos, sl, sr);
      l += w * (sl * g.gLL + sr * g.gRL);
      r += w * (sr * g.gRR + sl * g.gLR);
      g.pos += g.ratio;
      if (g.pos >= mBufLen) g.pos -= mBufLen;
      if (++g.age >= g.len) { g.active = false; g.env = 0.f; }
    }

    if (++mWrite >= mBufLen) mWrite = 0;
    outL = l * mNorm;
    outR = r * mNorm;
  }

  // For the visualizer: per-grain envelope (0..1), pan (0..1) and pitch (semitones)
  void Snapshot(float* env, float* pan, float* pitch) const
  {
    for (int i = 0; i < kMaxGrains; i++)
    {
      env[i] = mGrains[i].active ? mGrains[i].env : 0.f;
      pan[i] = (float) mGrains[i].pan;
      pitch[i] = (float) mGrains[i].semis;
    }
  }

  uint32_t TriggerCount() const { return mTriggerCount; }

private:
  struct Grain
  {
    bool active = false;
    bool chop = false;
    double pos = 0.0, ratio = 1.0, invLen = 1.0, fadeLen = 1.0, relLen = 1.0, decayRate = 0.0;
    int age = 0, len = 1;
    double gLL = 1.0, gRL = 0.0, gRR = 1.0, gLR = 0.0;
    double pan = 0.5, semis = 0.0;
    float env = 0.f;
  };

  double Window(const Grain& g) const
  {
    const double a = g.age, b = g.len - 1 - g.age;
    if (!g.chop)
    {
      // free grain: ~5 ms attack, flat, longer release
      if (a < g.fadeLen) return 0.5 - 0.5 * std::cos(kPi * a / g.fadeLen);
      if (b < g.relLen) return 0.5 - 0.5 * std::cos(kPi * b / g.relLen);
      return 1.0;
    }
    // stab: ~1 ms attack, punchy exponential decay, short release at the gate point
    const double attack = a < g.fadeLen ? 0.5 - 0.5 * std::cos(kPi * a / g.fadeLen) : 1.0;
    const double release = b < g.relLen ? 0.5 - 0.5 * std::cos(kPi * b / g.relLen) : 1.0;
    const double decay = 0.45 + 0.55 * std::exp(-a * g.decayRate);
    return attack * release * decay;
  }

  void ReadStereo(double pos, double& l, double& r) const
  {
    const int i0 = (int) pos;
    const int i1 = (i0 + 1 >= mBufLen) ? 0 : i0 + 1;
    const double f = pos - i0;
    l = mBufL[i0] + f * (mBufL[i1] - mBufL[i0]);
    r = mBufR[i0] + f * (mBufR[i1] - mBufR[i0]);
  }

  Grain* FreeGrain()
  {
    for (auto& g : mGrains) if (!g.active) return &g;
    return nullptr;
  }

  // Places a grain in the buffer: it never overtakes the write head (pitched up)
  // and never drifts further back than the buffer holds (pitched down).
  double StartPos(Grain& g, double extraDelay)
  {
    const double budget = mBufLen * 0.45;
    if (g.ratio > 1.0 && g.len * (g.ratio - 1.0) > budget)
    {
      g.len = std::max(16, (int) (budget / (g.ratio - 1.0)));
      g.invLen = 1.0 / g.len;
    }
    const double catchUp = g.len * std::max(0.0, g.ratio - 1.0);
    const double drift = g.len * std::max(0.0, 1.0 - g.ratio);
    const double maxDelay = mBufLen - 8.0 - drift;
    const double delay = Clamp(catchUp + extraDelay + 64.0, catchUp + 64.0, maxDelay);
    double pos = mWrite - delay;
    while (pos < 0.0) pos += mBufLen;
    return pos;
  }

  void SetPan(Grain& g, double pan)
  {
    g.pan = pan;
    const double a = pan * kPi * 0.5;
    const double gl = std::cos(a) * 1.41421356, gr = std::sin(a) * 1.41421356;
    g.gLL = std::min(1.0, gl); g.gRL = std::max(0.0, gl - 1.0);
    g.gRR = std::min(1.0, gr); g.gLR = std::max(0.0, gr - 1.0);
  }

  void SpawnCloudGrain()
  {
    Grain* p = FreeGrain();
    if (!p) return;
    Grain& g = *p;
    g.chop = false;
    g.len = std::max(16, (int) (mSizeSamples * (1.0 + 0.35 * mChaos * mRand.Bipolar())));
    g.invLen = 1.0 / g.len;
    g.age = 0;

    // detune jitter grows with chaos; occasional octave-up shimmer grains at high chaos
    g.semis = mPitch + mChaos * 0.35 * mRand.Bipolar();
    if (mChaos > 0.6 && mRand.Next() < (mChaos - 0.6) * 0.5) g.semis += 12.0;
    g.ratio = std::pow(2.0, g.semis / 12.0);

    g.fadeLen = std::min(0.005 * mSR, g.len * 0.25);
    g.relLen = g.len * 0.4;
    // small time scatter only (keeps it tight to the beat); grows with chaos
    const double scatter = mChaos * mChaos * mRand.Next() * mSR * 0.04;
    g.pos = StartPos(g, scatter);
    SetPan(g, 0.5 + 0.5 * mChaos * mRand.Bipolar());
    g.active = true;
    mTriggerCount++;
  }

  void SpawnChop()
  {
    // a new chop replaces the previous one (fade it out quickly instead of cutting)
    for (auto& o : mGrains)
      if (o.active && o.chop && o.len - o.age > (int) o.fadeLen)
        o.len = o.age + (int) o.fadeLen;

    Grain* p = FreeGrain();
    if (!p) return;
    Grain& g = *p;
    g.chop = true;
    const double maxLen = mBufLen * 0.45;
    g.len = std::max(64, (int) std::min(mSizeSamples * std::clamp(mGate, 0.05, 1.0), maxLen));
    g.invLen = 1.0 / g.len;
    g.fadeLen = std::min(0.0012 * mSR, g.len * 0.2);
    g.relLen = std::min(0.006 * mSR, g.len * 0.3);
    g.decayRate = 2.5 / g.len;
    g.age = 0;

    g.semis = mPitch;
    if (mChaos > 0.4 && mRand.Next() < (mChaos - 0.4) * 0.4) g.semis += (mRand.Next() < 0.5 ? 12.0 : -12.0);
    g.ratio = std::pow(2.0, g.semis / 12.0);

    // stutter: with chaos, sometimes repeat the previous slice instead of taking a new one
    const bool repeat = mLastChopPos >= 0.0 && mRand.Next() < mChaos * 0.6;
    if (repeat)
    {
      g.pos = mLastChopPos;
    }
    else
    {
      // read the live signal; chaos sometimes reaches back up to a step
      const double jitter = mRand.Next() < mChaos * 0.5 ? mRand.Next() * mSizeSamples : 0.0;
      g.pos = StartPos(g, jitter);
      mLastChopPos = g.pos;
    }
    SetPan(g, 0.5 + 0.35 * mChaos * mRand.Bipolar());
    g.active = true;
    mTriggerCount++;
  }

  double mSR = 44100.0;
  std::vector<float> mBufL, mBufR;
  int mBufLen = 1, mWrite = 0;
  double mSamplesToNext = 0.0, mLastChopPos = -1.0;
  bool mSync = false, mPendingTrigger = false;
  double mSizeSamples = 5000.0, mDensity = 16.0, mPitch = 0.0, mChaos = 0.3, mGate = 0.5, mNorm = 1.0;
  uint32_t mTriggerCount = 0;
  Grain mGrains[kMaxGrains];
  Random mRand { 0x1234567u };
};

// ---------------------------------------------------------------------------
// Plate reverb after Jon Dattorro, "Effect Design Part 1" (JAES 1997)
// ---------------------------------------------------------------------------
class PlateReverb
{
public:
  void Prepare(double sampleRate)
  {
    mSR = sampleRate;
    mScale = sampleRate / 29761.0;
    auto S = [&](double n) { return std::max(1, (int) std::lround(n * mScale)); };

    mPreDelay.Prepare((int) (sampleRate * 0.25) + 2);
    const int inLens[4] = { S(142), S(107), S(379), S(277) };
    for (int i = 0; i < 4; i++) { mInAP[i].Prepare(inLens[i]); mInLen[i] = inLens[i]; }

    mModAPLen[0] = S(672); mModAPLen[1] = S(908);
    mExcursion = 16.0 * mScale;
    mModAP[0].Prepare(mModAPLen[0] + (int) mExcursion + 4);
    mModAP[1].Prepare(mModAPLen[1] + (int) mExcursion + 4);

    mDel1Len[0] = S(4453); mDel1Len[1] = S(4217);
    mAP2Len[0] = S(1800);  mAP2Len[1] = S(2656);
    mDel2Len[0] = S(3720); mDel2Len[1] = S(3163);
    for (int t = 0; t < 2; t++)
    {
      mDel1[t].Prepare(mDel1Len[t]);
      mAP2[t].Prepare(mAP2Len[t]);
      mDel2[t].Prepare(mDel2Len[t]);
    }

    // Output taps (left uses tank nodes of the right side and vice versa)
    const int tL[7] = { 266, 2974, 1913, 1996, 1990, 187, 1066 };
    const int tR[7] = { 353, 3627, 1228, 2673, 2111, 335, 121 };
    for (int i = 0; i < 7; i++) { mTapL[i] = S(tL[i]); mTapR[i] = S(tR[i]); }

    mBandwidthState = 0.0;
    mDampState[0] = mDampState[1] = 0.0;
    mTankFeed[0] = mTankFeed[1] = 0.0;
    mLfoPhase = 0.0;
  }

  // decay 0..1, damping 0..1, preDelayMs
  void SetParams(double decay, double damping, double preDelayMs)
  {
    mDecay = 0.25 + 0.72 * decay;
    mDamping = 0.02 + 0.85 * damping;
    mPreDelaySamples = Clamp(preDelayMs * 0.001 * mSR, 1.0, mSR * 0.25);
    mDecayDiff2 = Clamp(mDecay + 0.15, 0.25, 0.5);
  }

  void Process(double inL, double inR, double& outL, double& outR)
  {
    mPreDelay.Write(0.5 * (inL + inR));
    double x = mPreDelay.ReadLinear(mPreDelaySamples);

    // Input bandwidth filter
    mBandwidthState += 0.9995 * (x - mBandwidthState);
    x = mBandwidthState;

    // Input diffusion
    x = AllPass(mInAP[0], mInLen[0], 0.75, x);
    x = AllPass(mInAP[1], mInLen[1], 0.75, x);
    x = AllPass(mInAP[2], mInLen[2], 0.625, x);
    x = AllPass(mInAP[3], mInLen[3], 0.625, x);

    mLfoPhase += 1.0 / mSR;
    if (mLfoPhase >= 1.0) mLfoPhase -= 1.0;
    const double lfo = std::sin(kTwoPi * mLfoPhase);
    const double mod[2] = { mExcursion * (1.0 + lfo) * 0.5, mExcursion * (1.0 - lfo) * 0.5 };

    double ends[2];
    for (int t = 0; t < 2; t++)
    {
      double v = x + mTankFeed[1 - t];
      // Modulated decay-diffusion allpass (negative coefficient per the paper)
      {
        const double z = mModAP[t].ReadLinear(mModAPLen[t] + mod[t]);
        const double w = v + 0.7 * z;
        mModAP[t].Write(w + 1e-24);
        v = z - 0.7 * w;
      }
      mDel1[t].Write(v);
      v = mDel1[t].Read(mDel1Len[t]);
      mDampState[t] += (1.0 - mDamping) * (v - mDampState[t]);
      v = mDampState[t] * mDecay;
      v = AllPass(mAP2[t], mAP2Len[t], mDecayDiff2, v);
      mDel2[t].Write(v);
      ends[t] = mDel2[t].Read(mDel2Len[t]) * mDecay;
    }
    mTankFeed[0] = ends[0];
    mTankFeed[1] = ends[1];

    // tank index 0 = "left" half (nodes 24..39), 1 = "right" half (nodes 48..63)
    double l = 0.0, r = 0.0;
    l += mDel1[1].Read(mTapL[0]);
    l += mDel1[1].Read(mTapL[1]);
    l -= mAP2[1].Read(mTapL[2]);
    l += mDel2[1].Read(mTapL[3]);
    l -= mDel1[0].Read(mTapL[4]);
    l -= mAP2[0].Read(mTapL[5]);
    l -= mDel2[0].Read(mTapL[6]);

    r += mDel1[0].Read(mTapR[0]);
    r += mDel1[0].Read(mTapR[1]);
    r -= mAP2[0].Read(mTapR[2]);
    r += mDel2[0].Read(mTapR[3]);
    r -= mDel1[1].Read(mTapR[4]);
    r -= mAP2[1].Read(mTapR[5]);
    r -= mDel2[1].Read(mTapR[6]);

    outL = l * 0.42;
    outR = r * 0.42;
  }

private:
  static double AllPass(DelayLine& d, int len, double g, double x)
  {
    const double z = d.Read(len);
    const double w = x - g * z;
    d.Write(w + 1e-24);
    return z + g * w;
  }

  double mSR = 44100.0, mScale = 1.0;
  double mDecay = 0.5, mDamping = 0.3, mDecayDiff2 = 0.5, mPreDelaySamples = 1.0, mExcursion = 16.0;
  double mBandwidthState = 0.0, mDampState[2] = {}, mTankFeed[2] = {}, mLfoPhase = 0.0;
  DelayLine mPreDelay, mInAP[4], mModAP[2], mDel1[2], mAP2[2], mDel2[2];
  int mInLen[4] = {}, mModAPLen[2] = {}, mDel1Len[2] = {}, mAP2Len[2] = {}, mDel2Len[2] = {};
  int mTapL[7] = {}, mTapR[7] = {};
};

// ---------------------------------------------------------------------------
// Mid/side stereo widener with mono-safe lows
// ---------------------------------------------------------------------------
class Widener
{
public:
  void Prepare(double sampleRate) { mLpCoeff = 1.0 - std::exp(-kTwoPi * 160.0 / sampleRate); mLow = 0.0; }

  void Process(double width, double& l, double& r)
  {
    const double m = 0.5 * (l + r);
    const double s = 0.5 * (l - r);
    mLow += mLpCoeff * (s - mLow);
    const double sHigh = s - mLow;
    const double sOut = mLow * std::min(width, 1.0) + sHigh * width;
    l = m + sOut;
    r = m - sOut;
  }

private:
  double mLpCoeff = 0.02, mLow = 0.0;
};

} // namespace ambi
