#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ThreeBodyEngine.h"
#include "WavetableOscillator.h"
#include "WaveformPreviewControl.h"
#include "ADSREnvelope.h"
#include <atomic>

// ============================================================================
// Config attendue dans TroisCorpsWave/config.h :
//
//   #define PLUG_TYPE 1              // Instrument
//   #define PLUG_DOES_MIDI_IN 1
//   #define PLUG_DOES_MIDI_OUT 0
//   #define PLUG_CHANNEL_IO "0-2"    // pas d'entree audio, sortie stereo (vrai son)
// ============================================================================

enum EParams
{
  kParamMass1 = 0,
  kParamMass2,
  kParamMass3,
  kParamRadius1,
  kParamRadius2,
  kParamRadius3,
  kParamAngle1,
  kParamAngle2,
  kParamAngle3,
  kParamOrbitalVelocity,
  kParamBoxSize,
  kParamCaptureWindow,
  kParamTableSize,
  kParamBitDepth,
  kParamVol1,       // volume independant de l'oscillateur du corps 1
  kParamVol2,
  kParamVol3,
  kParamAttack,     // enveloppe ADSR (partagee par les 3 oscillateurs)
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class TroisCorpsWave final : public iplug::Plugin
{
public:
  TroisCorpsWave(const InstanceInfo& info);

  void OnIdle() override;
  void OnUIClose() override { mWaveformView = nullptr; }

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
#endif

private:
  void RequestCapture() { mCaptureRequested = true; }

  WaveformPreviewControl* mWaveformView = nullptr;
  std::atomic<bool> mTableUpdatedForUI { false };
  float mUITableCopy1[WavetableOscillator::kMaxTableSize] = { 0.f };
  float mUITableCopy2[WavetableOscillator::kMaxTableSize] = { 0.f };
  float mUITableCopy3[WavetableOscillator::kMaxTableSize] = { 0.f };
  int mUITableSize = 0;

#if IPLUG_DSP
  void DoCapture();
  void UpdateEnvelopeParams();

  ThreeBodyEngine mEngine;
  WavetableOscillator mOsc1, mOsc2, mOsc3;
  ADSREnvelope mEnv;

  std::atomic<bool> mCaptureRequested { false };

  static constexpr int kMaxRawCapture = 80000; // 10s a 8000Hz de resolution de capture
  float mRawCapture1[kMaxRawCapture];
  float mRawCapture2[kMaxRawCapture];
  float mRawCapture3[kMaxRawCapture];
#endif
};
