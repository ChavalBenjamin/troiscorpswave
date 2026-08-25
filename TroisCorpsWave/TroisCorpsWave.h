#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ThreeBodyEngine.h"
#include "WavetableOscillator.h"
#include <atomic>

// ============================================================================
// Config attendue dans TroisCorpsWave/config.h :
//
//   #define PLUG_TYPE 1              // Instrument
//   #define PLUG_DOES_MIDI_IN 1
//   #define PLUG_DOES_MIDI_OUT 0
//   #define PLUG_CHANNEL_IO "0-2"    // pas d'entree audio, sortie stereo (vrai son cette fois)
// ============================================================================

enum EParams
{
  kParamMass1 = 0,
  kParamMass2,
  kParamMass3,
  kParamStartRadius,   // taille du triangle de depart
  kParamBoxSize,       // taille de la zone bornee (rebond sur les bords)
  kParamCaptureWindow, // duree simulee capturee, en secondes
  kParamTableSize,     // taille de la table d'onde (puissance de 2)
  kParamBitDepth,      // reduction de bits (2-16)
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class TroisCorpsWave final : public iplug::Plugin
{
public:
  TroisCorpsWave(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
#endif

private:
  void RequestCapture() { mCaptureRequested = true; }

#if IPLUG_DSP
  void DoCapture();

  ThreeBodyEngine mEngine;
  WavetableOscillator mOsc;

  std::atomic<bool> mCaptureRequested { false };

  static constexpr int kMaxRawCapture = 20000; // ~2.5s a 8000Hz de resolution de capture
  float mRawCaptureBuffer[kMaxRawCapture];

  // Tres court fondu d'entree/sortie autour des Note On/Off pour eviter
  // les clics (pas une vraie enveloppe ADSR - ca viendra a une etape suivante).
  float mFadeGain = 0.f;
  float mFadeTarget = 0.f;
  static constexpr float kFadeStep = 1.f / 200.f; // ~4.5ms a 44.1kHz
#endif
};
