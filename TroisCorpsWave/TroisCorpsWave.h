#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ThreeBodyEngine.h"
#include "MorphingOscillator.h"
#include "WaveformPreviewControl.h"
#include "ADSREnvelope.h"
#include <atomic>

// ============================================================================
// Config attendue dans TroisCorpsWave/config.h :
//
//   #define PLUG_TYPE 1
//   #define PLUG_DOES_MIDI_IN 1
//   #define PLUG_DOES_MIDI_OUT 0
//   #define PLUG_CHANNEL_IO "0-2"
// ============================================================================

// 13 parametres physiques par banque, dans cet ordre :
enum EBankParamOffset
{
  kOffMass1 = 0, kOffMass2, kOffMass3,
  kOffRadius1, kOffRadius2, kOffRadius3,
  kOffAngle1, kOffAngle2, kOffAngle3,
  kOffOrbitalVel, kOffBoxSize, kOffCaptureWindow, kOffTableSize,
  kNumBankParams
};

enum EParams
{
  kParamBank0Start = 0,
  kParamBank1Start = kParamBank0Start + kNumBankParams,
  kParamBank2Start = kParamBank1Start + kNumBankParams,

  kParamVol1 = kParamBank2Start + kNumBankParams,
  kParamVol2,
  kParamVol3,
  kParamAttack,
  kParamDecay,
  kParamSustain,
  kParamRelease,
  kParamBitDepth,
  kParamMorph,

  kNumParams
};

// Index du parametre "offset" dans la banque "bankIdx" (0,1,2)
inline int BankParam(int bankIdx, int offset) { return kParamBank0Start + bankIdx * kNumBankParams + offset; }

using namespace iplug;
using namespace igraphics;

class TroisCorpsWave final : public iplug::Plugin
{
public:
  TroisCorpsWave(const InstanceInfo& info);

  void OnIdle() override;
  void OnUIClose() override
  {
    for (int b = 0; b < 3; b++) mBankWaveView[b] = nullptr;
    mMorphWaveView = nullptr;
  }

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
#endif

private:
  void SwitchTab(int tabIdx);
  void RequestCaptureActiveTab() { mForceCaptureBank = mActiveTab; }

  int mActiveTab = 0;
  IControl* mBankControls[3][kNumBankParams] = {};   // pour montrer/cacher par onglet
  WaveformPreviewControl* mBankWaveView[3] = { nullptr, nullptr, nullptr };
  WaveformPreviewControl* mMorphWaveView = nullptr;

  // Etat partage thread audio -> interface, par banque (apercu par onglet)
  std::atomic<bool> mBankUIUpdated[3] = { false, false, false };
  float mBankUICopy1[3][MorphingOscillator::kMaxTableSize] = {};
  float mBankUICopy2[3][MorphingOscillator::kMaxTableSize] = {};
  float mBankUICopy3[3][MorphingOscillator::kMaxTableSize] = {};
  int mBankUISize[3] = { 0, 0, 0 };

  std::atomic<float> mUIMorphPosition { 0.f };

#if IPLUG_DSP
  void DoCaptureBank(int bankIdx);
  void UpdateEnvelopeParams();

  ThreeBodyEngine mEngine;
  MorphingOscillator mOsc1, mOsc2, mOsc3;
  ADSREnvelope mEnv;

  std::atomic<int> mForceCaptureBank { -1 }; // -1 = rien en attente ; sinon index de banque a forcer (bouton Capture)

  static constexpr int kMaxRawCapture = 80000; // 10s a 8000Hz de resolution de capture
  float mRawCapture1[kMaxRawCapture];
  float mRawCapture2[kMaxRawCapture];
  float mRawCapture3[kMaxRawCapture];

  // Delai independant par banque avant recapture automatique (voir
  // OnParamChange / ProcessBlock) : chaque banque a son propre compte a
  // rebours, pour ne recapturer que la banque effectivement modifiee.
  int mBankDebounceSamples[3] = { 0, 0, 0 };
  static constexpr double kDebounceMs = 80.0;
#endif
};
