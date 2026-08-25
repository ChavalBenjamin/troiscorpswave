#pragma once

#include <cmath>
#include <algorithm>

// ============================================================================
// MorphingOscillator
//
// Contient 3 tables d'onde independantes (une par "banque"), et melange en
// temps reel entre elles selon une position de morph (0-127) :
//   0-63   : fondu entre banque 0 et banque 1
//   63-127 : fondu entre banque 1 et banque 2
// Le melange est calcule echantillon par echantillon (vrai morphing continu,
// pas une recapture a chaque mouvement du slider).
//
// Si le contenu d'UNE banque change pendant qu'une note sonne, cette banque
// (et elle seule) beneficie d'un court fondu interne (~15ms) pour eviter
// tout clic - independant du morphing entre banques.
// ============================================================================

class MorphingOscillator
{
public:
  static constexpr int kMaxTableSize = 4096;

  // Met a jour le contenu de la banque "bankIdx" (0, 1 ou 2) pour cet
  // oscillateur. Reechantillonne, normalise, et lisse le raccord de boucle.
  void SetBankTable(int bankIdx, const float* source, int nSourceSamples, int tableSize)
  {
    if (bankIdx < 0 || bankIdx > 2 || nSourceSamples <= 1) return;

    tableSize = std::clamp(tableSize, 4, kMaxTableSize);
    float* dest = mActive ? mBankNextTable[bankIdx] : mBankTable[bankIdx];

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
      dest[i] = sample / maxAbs;
    }

    // Raccord de boucle (evite l'effet dents de scie a chaque tour)
    int fadeLen = std::max(2, tableSize / 20);
    for (int i = 0; i < fadeLen; i++)
    {
      float t = (float)i / (float)fadeLen;
      int idx = tableSize - fadeLen + i;
      dest[idx] = dest[idx] * (1.f - t) + dest[0] * t;
    }

    if (mActive)
    {
      mBankNextTableSize[bankIdx] = tableSize;
      mBankCrossfading[bankIdx] = true;
      mBankCrossfadeProgress[bankIdx] = 0.f;
      mBankCrossfadeInc[bankIdx] = 1.f / (float)(kCrossfadeMs * 0.001 * mSampleRate);
    }
    else
    {
      mBankTableSize[bankIdx] = tableSize;
    }
  }

  // Position de morph, 0-127. Lue en direct a chaque echantillon : aucun
  // traitement special necessaire, le changement est deja fluide par nature
  // (interpolation continue entre des tables deja stables).
  void SetMorphPosition(float pos0to127) { mMorphPosition = std::clamp(pos0to127, 0.f, 127.f); }

  void SetBitDepth(int bits) { mBitDepth = std::clamp(bits, 2, 16); }
  void SetSampleRate(double sr) { mSampleRate = std::max(1000.0, sr); }

  // Pour l'affichage : table "etablie" actuelle d'une banque donnee.
  const float* GetBankTable(int bankIdx) const { return mBankTable[bankIdx]; }
  int GetBankTableSize(int bankIdx) const { return mBankTableSize[bankIdx]; }

  void NoteOn(int midiNote)
  {
    mFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    mPhase = 0.0;
    mActive = true;
    for (int b = 0; b < 3; b++) mBankCrossfading[b] = false;
  }

  void NoteOff()
  {
    mActive = false;
    for (int b = 0; b < 3; b++) mBankCrossfading[b] = false;
  }

  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive) return 0.f;

    float bankSample[3];
    for (int b = 0; b < 3; b++)
      bankSample[b] = ReadBank(b);

    float sample;
    if (mMorphPosition <= 63.f)
    {
      float t = mMorphPosition / 63.f;
      sample = bankSample[0] * (1.f - t) + bankSample[1] * t;
    }
    else
    {
      float t = (mMorphPosition - 63.f) / 64.f;
      sample = bankSample[1] * (1.f - t) + bankSample[2] * t;
    }

    float levels = (float)(1 << mBitDepth);
    sample = std::round(sample * levels * 0.5f) / (levels * 0.5f);

    mPhase += mFreq / mSampleRate;
    if (mPhase >= 1.0) mPhase -= 1.0;

    return sample;
  }

private:
  float ReadTableAt(const float* table, int size) const
  {
    if (size <= 0) return 0.f;
    float phaseInTable = (float)mPhase * (float)size;
    int i0 = (int)phaseInTable % size;
    int i1 = (i0 + 1) % size;
    float frac = phaseInTable - (float)(int)phaseInTable;
    return table[i0] * (1.f - frac) + table[i1] * frac;
  }

  // Lit la banque bankIdx a la phase actuelle, en gerant son eventuel
  // fondu interne (avance l'etat de fondu - a appeler une seule fois par
  // banque par echantillon).
  float ReadBank(int bankIdx)
  {
    int size = mBankTableSize[bankIdx];
    if (size <= 0) return 0.f;

    float sample = ReadTableAt(mBankTable[bankIdx], size);

    if (mBankCrossfading[bankIdx])
    {
      int nextSize = mBankNextTableSize[bankIdx];
      float sampleNext = ReadTableAt(mBankNextTable[bankIdx], nextSize);
      sample = sample * (1.f - mBankCrossfadeProgress[bankIdx]) + sampleNext * mBankCrossfadeProgress[bankIdx];

      mBankCrossfadeProgress[bankIdx] += mBankCrossfadeInc[bankIdx];
      if (mBankCrossfadeProgress[bankIdx] >= 1.f)
      {
        std::copy(mBankNextTable[bankIdx], mBankNextTable[bankIdx] + nextSize, mBankTable[bankIdx]);
        mBankTableSize[bankIdx] = nextSize;
        mBankCrossfading[bankIdx] = false;
      }
    }

    return sample;
  }

  static constexpr float kCrossfadeMs = 15.f;

  float mBankTable[3][kMaxTableSize] = {};
  float mBankNextTable[3][kMaxTableSize] = {};
  int mBankTableSize[3] = { 0, 0, 0 };
  int mBankNextTableSize[3] = { 0, 0, 0 };
  bool mBankCrossfading[3] = { false, false, false };
  float mBankCrossfadeProgress[3] = { 0.f, 0.f, 0.f };
  float mBankCrossfadeInc[3] = { 0.f, 0.f, 0.f };

  float mMorphPosition = 0.f;

  double mPhase = 0.0;
  double mFreq = 440.0;
  double mSampleRate = 44100.0;
  bool mActive = false;
  int mBitDepth = 16;
};
