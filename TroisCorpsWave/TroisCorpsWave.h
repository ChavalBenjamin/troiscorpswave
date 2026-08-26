#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ThreeBodyEngine.h"
#include "MorphingOscillator.h"
#include "WaveformPreviewControl.h"
#include "BodyAnimationControl.h"
#include "ModulatableKnobControl.h"
#include "ADSREnvelope.h"
#include <atomic>

// ============================================================================
// Config attendue dans TroisCorpsWave/config.h :
//
//   #define PLUG_TYPE 1
//   #define PLUG_DOES_MIDI_IN 1
//   #define PLUG_DOES_MIDI_OUT 1    <-- active pour les 3 CC/LFO
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
  kParamActiveTab,  // 0/1/2 - onglet d'edition visible (IVTabSwitchControl)
  kParamLFOBank,    // 0/1/2 - banque source des 3 LFO/CC et de l'animation, INDEPENDANT de kParamActiveTab
  kParamCC1Number,
  kParamCC2Number,
  kParamCC3Number,
  kParamLFORate, // 0.001 - 1 : ralentit la boucle LFO/animation par rapport a sa vitesse max

  // Petite matrice de modulation : chaque destination peut etre pilotee en
  // direct par la position d'un des 3 corps de la banque LFO selectionnee
  // (ou aucun, par defaut).
  kParamModVol1Src,
  kParamModVol2Src,
  kParamModVol3Src,
  kParamModMorphSrc,

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
    mAnimView = nullptr;
    mModVol1Knob = nullptr;
    mModVol2Knob = nullptr;
    mModVol3Knob = nullptr;
    mModMorphSlider = nullptr;
  }

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
#endif

private:
  void SwitchTab(int tabIdx);

  int mActiveTab = 0;
  IControl* mBankControls[3][kNumBankParams] = {};   // pour montrer/cacher par onglet
  WaveformPreviewControl* mBankWaveView[3] = { nullptr, nullptr, nullptr };
  WaveformPreviewControl* mMorphWaveView = nullptr;
  BodyAnimationControl* mAnimView = nullptr;

  ModulatableKnobControl* mModVol1Knob = nullptr;
  ModulatableKnobControl* mModVol2Knob = nullptr;
  ModulatableKnobControl* mModVol3Knob = nullptr;
  ModulatableSliderControl* mModMorphSlider = nullptr;
  std::atomic<float> mUIModVol1Value { 0.f };
  std::atomic<float> mUIModVol2Value { 0.f };
  std::atomic<float> mUIModVol3Value { 0.f };
  std::atomic<float> mUIModMorphValue { 0.f };

  // Etat partage thread audio -> interface, par banque (apercu par onglet)
  std::atomic<bool> mBankUIUpdated[3] = { false, false, false };
  float mBankUICopy1[3][MorphingOscillator::kMaxTableSize] = {};
  float mBankUICopy2[3][MorphingOscillator::kMaxTableSize] = {};
  float mBankUICopy3[3][MorphingOscillator::kMaxTableSize] = {};
  int mBankUISize[3] = { 0, 0, 0 };

  std::atomic<float> mUIMorphPosition { 0.f };
  float mLastDrawnMorph = -1.f;

  // Positions courantes des 3 corps (banque LFO active), pour l'animation.
  std::atomic<double> mUIBodyX[3] { 0.0, 0.0, 0.0 };
  std::atomic<double> mUIBodyY[3] { 0.0, 0.0, 0.0 };
  std::atomic<double> mUIAnimScale { 1.0 };

#if IPLUG_DSP
  void DoCaptureBank(int bankIdx);
  void UpdateEnvelopeParams();

  ThreeBodyEngine mEngine;
  MorphingOscillator mOsc1, mOsc2, mOsc3;
  ADSREnvelope mEnv;

  static constexpr int kMaxRawCapture = 80000; // 10s a 8000Hz de resolution de capture
  float mRawX1[kMaxRawCapture]; float mRawY1[kMaxRawCapture];
  float mRawX2[kMaxRawCapture]; float mRawY2[kMaxRawCapture];
  float mRawX3[kMaxRawCapture]; float mRawY3[kMaxRawCapture];

  // Donnees persistantes (X,Y par corps, par banque) pour l'animation et
  // les LFO/CC - reechantillonnees a une resolution fixe, independantes de
  // la table audio (qui peut avoir une toute autre taille selon Table Size).
  static constexpr int kAnimRes = 2048;
  float mBankAnimX[3][3][kAnimRes] = {}; // [banque][corps][echantillon]
  float mBankAnimY[3][3][kAnimRes] = {};
  float mBankAnimScale[3] = { 1.f, 1.f, 1.f }; // echelle d'affichage (etendue max), par banque

  int mLastSentCC1 = -1, mLastSentCC2 = -1, mLastSentCC3 = -1;

  // Phase du LFO/de l'animation : boucle libre en continu, calee sur la
  // duree reelle (Capture Window) de la banque actuellement selectionnee
  // pour le LFO - independant des notes jouees.
  double mLFOPhase = 0.0;

  // Delai independant par banque avant recapture automatique (voir
  // OnParamChange / ProcessBlock) : chaque banque a son propre compte a
  // rebours, pour ne recapturer que la banque effectivement modifiee.
  int mBankDebounceSamples[3] = { 0, 0, 0 };
  static constexpr double kDebounceMs = 80.0;
#endif
};
