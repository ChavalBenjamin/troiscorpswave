#pragma once

#include <cmath>
#include <algorithm>

// ============================================================================
// WavetableOscillator
//
// Rejoue en boucle une table d'onde (capturee depuis le moteur physique),
// a la hauteur correspondant a la note MIDI jouee. Inclut un reechantillonnage
// propre (interpolation lineaire) vers une taille de table reglable, une
// normalisation d'amplitude, et une reduction de bits optionnelle (bitcrush).
// ============================================================================

class WavetableOscillator
{
public:
  static constexpr int kMaxTableSize = 4096;

  // Recopie "source" (nSourceSamples points) dans la table interne,
  // reechantillonnee a "tableSize" points par interpolation lineaire, et
  // normalisee en amplitude (-1..1).
  void SetTable(const float* source, int nSourceSamples, int tableSize)
  {
    if (nSourceSamples <= 1) return;

    tableSize = std::clamp(tableSize, 4, kMaxTableSize);
    mTableSize = tableSize;

    float maxAbs = 1e-9f;
    for (int i = 0; i < nSourceSamples; i++)
      maxAbs = std::max(maxAbs, std::abs(source[i]));

    for (int i = 0; i < tableSize; i++)
    {
      float srcPos = (float)i / (float)tableSize * (float)nSourceSamples;
      int i0 = (int)srcPos;
      int i1 = std::min(i0 + 1, nSourceSamples - 1);
      float frac = srcPos - (float)i0;
      float sample = source[i0] * (1.f - frac) + source[i1] * frac;
      mTable[i] = sample / maxAbs;
    }
  }

  void SetBitDepth(int bits) { mBitDepth = std::clamp(bits, 2, 16); }
  void SetSampleRate(double sr) { mSampleRate = sr; }

  void NoteOn(int midiNote)
  {
    mFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    mPhase = 0.0;
    mActive = true;
  }

  void NoteOff() { mActive = false; }
  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive || mTableSize <= 0) return 0.f;

    float phaseInTable = (float)mPhase * (float)mTableSize;
    int i0 = (int)phaseInTable % mTableSize;
    int i1 = (i0 + 1) % mTableSize;
    float frac = phaseInTable - (float)(int)phaseInTable;
    float sample = mTable[i0] * (1.f - frac) + mTable[i1] * frac;

    // Reduction de bits (quantification de l'amplitude) - a 16 bits c'est
    // quasi transparent, plus le chiffre baisse plus l'effet "lo-fi" est marque.
    float levels = (float)(1 << mBitDepth);
    sample = std::round(sample * levels * 0.5f) / (levels * 0.5f);

    mPhase += mFreq / mSampleRate;
    if (mPhase >= 1.0) mPhase -= 1.0;

    return sample;
  }

private:
  float mTable[kMaxTableSize] = { 0.f };
  int mTableSize = 0;

  double mPhase = 0.0;
  double mFreq = 440.0;
  double mSampleRate = 44100.0;
  bool mActive = false;
  int mBitDepth = 16;
};
