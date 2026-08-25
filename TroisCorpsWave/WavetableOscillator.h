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
//
// Si SetTable() est appelee PENDANT qu'une note sonne (ex. l'utilisateur
// tourne un bouton en temps reel), la nouvelle table n'est pas appliquee
// brutalement (ce qui creerait un clic) : elle est fondue en douceur avec
// l'ancienne sur ~15ms (crossfade), evitant tout saut audible.
// ============================================================================

class WavetableOscillator
{
public:
  static constexpr int kMaxTableSize = 4096;

  // Recopie "source" (nSourceSamples points) dans la table, reechantillonnee
  // a "tableSize" points par interpolation lineaire, normalisee (-1..1), et
  // avec un raccord de boucle lisse. Si l'oscillateur joue deja une note,
  // la nouvelle table est appliquee via un court crossfade plutot qu'un
  // remplacement brutal.
  void SetTable(const float* source, int nSourceSamples, int tableSize)
  {
    if (nSourceSamples <= 1) return;

    tableSize = std::clamp(tableSize, 4, kMaxTableSize);
    float* dest = mActive ? mNextTable : mTable;

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

    // Raccord de boucle : lisse la jonction fin -> debut (evite l'effet
    // dents de scie d'un saut net a chaque tour de boucle).
    int fadeLen = std::max(2, tableSize / 20);
    for (int i = 0; i < fadeLen; i++)
    {
      float t = (float)i / (float)fadeLen;
      int idx = tableSize - fadeLen + i;
      dest[idx] = dest[idx] * (1.f - t) + dest[0] * t;
    }

    if (mActive)
    {
      // Une note sonne deja : on ne remplace pas mTable directement (saut
      // audible), on programme un court fondu vers cette nouvelle table.
      mNextTableSize = tableSize;
      mCrossfading = true;
      mCrossfadeProgress = 0.f;
      mCrossfadeInc = 1.f / (float)(kCrossfadeMs * 0.001 * mSampleRate);
    }
    else
    {
      // Rien ne joue : on peut appliquer directement, aucun risque de clic.
      mTableSize = tableSize;
    }
  }

  void SetBitDepth(int bits) { mBitDepth = std::clamp(bits, 2, 16); }
  void SetSampleRate(double sr) { mSampleRate = std::max(1000.0, sr); }

  // Pour l'affichage (WaveformPreviewControl) : la table "etablie"
  // actuellement (peut avoir un tres leger retard visuel de ~15ms sur le
  // dernier changement si un crossfade est en cours - sans consequence pour
  // un affichage statique).
  const float* GetTable() const { return mTable; }
  int GetTableSize() const { return mTableSize; }

  void NoteOn(int midiNote)
  {
    mFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    mPhase = 0.0;
    mActive = true;
    mCrossfading = false; // nouvelle note : on repart net sur la table etablie
  }

  void NoteOff() { mActive = false; mCrossfading = false; }
  bool IsActive() const { return mActive; }

  float Process()
  {
    if (!mActive || mTableSize <= 0) return 0.f;

    float sample = ReadTable(mTable, mTableSize);

    if (mCrossfading)
    {
      float sampleNext = ReadTable(mNextTable, mNextTableSize);
      sample = sample * (1.f - mCrossfadeProgress) + sampleNext * mCrossfadeProgress;

      mCrossfadeProgress += mCrossfadeInc;
      if (mCrossfadeProgress >= 1.f)
      {
        // Le fondu est termine : la nouvelle table devient la table active.
        std::copy(mNextTable, mNextTable + mNextTableSize, mTable);
        mTableSize = mNextTableSize;
        mCrossfading = false;
      }
    }

    // Reduction de bits (quantification de l'amplitude) - a 16 bits c'est
    // quasi transparent, plus le chiffre baisse plus l'effet "lo-fi" est marque.
    float levels = (float)(1 << mBitDepth);
    sample = std::round(sample * levels * 0.5f) / (levels * 0.5f);

    mPhase += mFreq / mSampleRate;
    if (mPhase >= 1.0) mPhase -= 1.0;

    return sample;
  }

private:
  float ReadTable(const float* table, int size) const
  {
    float phaseInTable = (float)mPhase * (float)size;
    int i0 = (int)phaseInTable % size;
    int i1 = (i0 + 1) % size;
    float frac = phaseInTable - (float)(int)phaseInTable;
    return table[i0] * (1.f - frac) + table[i1] * frac;
  }

  static constexpr float kCrossfadeMs = 15.f;

  float mTable[kMaxTableSize] = { 0.f };
  float mNextTable[kMaxTableSize] = { 0.f };
  int mTableSize = 0;
  int mNextTableSize = 0;

  bool mCrossfading = false;
  float mCrossfadeProgress = 0.f;
  float mCrossfadeInc = 0.f;

  double mPhase = 0.0;
  double mFreq = 440.0;
  double mSampleRate = 44100.0;
  bool mActive = false;
  int mBitDepth = 16;
};
