#pragma once

#include <algorithm>

// ============================================================================
// ADSREnvelope
//
// Enveloppe d'amplitude classique a 4 etages (Attack/Decay/Sustain/Release),
// declenchee par NoteOn()/NoteOff(). Le niveau courant sert de multiplicateur
// applique au signal audio (0..1).
// ============================================================================

class ADSREnvelope
{
public:
  enum class Stage { Idle, Attack, Decay, Sustain, Release };

  void SetSampleRate(double sr) { mSampleRate = std::max(1000.0, sr); }

  void SetADSR(double attackSec, double decaySec, double sustainLevel, double releaseSec)
  {
    mAttackSec = std::max(0.001, attackSec);
    mDecaySec = std::max(0.001, decaySec);
    mSustainLevel = std::clamp(sustainLevel, 0.0, 1.0);
    mReleaseSec = std::max(0.001, releaseSec);
  }

  void NoteOn()
  {
    mStage = Stage::Attack;
    mAttackInc = 1.f / (float)(mAttackSec * mSampleRate);
  }

  void NoteOff()
  {
    if (mStage != Stage::Idle)
    {
      mReleaseStep = mLevel / (float)(mReleaseSec * mSampleRate);
      mStage = Stage::Release;
    }
  }

  bool IsActive() const { return mStage != Stage::Idle; }

  float Process()
  {
    switch (mStage)
    {
      case Stage::Attack:
        mLevel += mAttackInc;
        if (mLevel >= 1.f)
        {
          mLevel = 1.f;
          mStage = Stage::Decay;
          mDecayDec = (1.f - (float)mSustainLevel) / (float)(mDecaySec * mSampleRate);
        }
        break;

      case Stage::Decay:
        mLevel -= mDecayDec;
        if (mLevel <= (float)mSustainLevel)
        {
          mLevel = (float)mSustainLevel;
          mStage = Stage::Sustain;
        }
        break;

      case Stage::Sustain:
        mLevel = (float)mSustainLevel;
        break;

      case Stage::Release:
        mLevel -= mReleaseStep;
        if (mLevel <= 0.f)
        {
          mLevel = 0.f;
          mStage = Stage::Idle;
        }
        break;

      case Stage::Idle:
      default:
        mLevel = 0.f;
        break;
    }
    return mLevel;
  }

private:
  double mSampleRate = 44100.0;
  double mAttackSec = 0.01;
  double mDecaySec = 0.3;
  double mSustainLevel = 0.7;
  double mReleaseSec = 0.3;

  Stage mStage = Stage::Idle;
  float mLevel = 0.f;
  float mAttackInc = 0.f;
  float mDecayDec = 0.f;
  float mReleaseStep = 0.f;
};
