#pragma once

#include "IControl.h"
#include <algorithm>

// ============================================================================
// WaveformPreviewControl
//
// Affiche la derniere table d'onde capturee sous forme de courbe statique
// (pas une animation temps reel - juste un instantane, mis a jour a chaque
// Capture). Permet de voir si le contenu capture est une vraie oscillation
// ou plutot une rampe/derive (ce qui expliquerait un son type dents de scie).
// ============================================================================

class WaveformPreviewControl : public iplug::igraphics::IControl
{
public:
  WaveformPreviewControl(const iplug::igraphics::IRECT& bounds)
  : IControl(bounds)
  {
  }

  // A appeler depuis le thread interface (OnIdle), jamais depuis l'audio.
  void SetWaveform(const float* table, int tableSize)
  {
    mSize = std::min(tableSize, kMaxPoints);
    for (int i = 0; i < mSize; i++)
      mBuffer[i] = table[i];
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    using namespace iplug::igraphics;

    g.FillRect(IColor(255, 15, 15, 20), mRECT);

    // Ligne du zero, pour reperer visuellement une derive vs une vraie oscillation
    float midY = mRECT.MH();
    g.DrawLine(IColor(255, 60, 60, 70), mRECT.L, midY, mRECT.R, midY, nullptr, 1.f);

    if (mSize < 2) return;

    float w = mRECT.W();
    float h = mRECT.H() * 0.45f; // demi-hauteur utilisee, laisse une marge haut/bas

    for (int i = 0; i < mSize - 1; i++)
    {
      float x0 = mRECT.L + w * (float)i / (float)(mSize - 1);
      float x1 = mRECT.L + w * (float)(i + 1) / (float)(mSize - 1);
      float y0 = midY - mBuffer[i] * h;
      float y1 = midY - mBuffer[i + 1] * h;
      g.DrawLine(IColor(255, 120, 200, 255), x0, y0, x1, y1, nullptr, 1.5f);
    }
  }

private:
  static constexpr int kMaxPoints = 4096;
  float mBuffer[kMaxPoints] = { 0.f };
  int mSize = 0;
};
