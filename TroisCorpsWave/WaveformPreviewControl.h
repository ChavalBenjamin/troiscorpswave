#pragma once

#include "IControl.h"
#include <algorithm>

// ============================================================================
// WaveformPreviewControl
//
// Affiche les 3 tables d'onde capturees (une par corps) superposees, sous
// forme de courbes statiques (pas d'animation temps reel - juste un
// instantane, mis a jour a chaque Capture). Permet de voir si le contenu
// capture est une vraie oscillation ou plutot une rampe/derive, et de
// comparer visuellement la richesse relative des 3 corps.
// ============================================================================

class WaveformPreviewControl : public iplug::igraphics::IControl
{
public:
  WaveformPreviewControl(const iplug::igraphics::IRECT& bounds)
  : IControl(bounds)
  {
  }

  // A appeler depuis le thread interface (OnIdle), jamais depuis l'audio.
  void SetWaveforms(const float* table1, const float* table2, const float* table3, int tableSize)
  {
    mSize = std::min(tableSize, kMaxPoints);
    for (int i = 0; i < mSize; i++)
    {
      mBuffer1[i] = table1[i];
      mBuffer2[i] = table2[i];
      mBuffer3[i] = table3[i];
    }
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    using namespace iplug::igraphics;

    g.FillRect(IColor(255, 15, 15, 20), mRECT);

    float midY = mRECT.MH();
    g.DrawLine(IColor(255, 60, 60, 70), mRECT.L, midY, mRECT.R, midY, nullptr, 1.f);

    if (mSize < 2) return;

    DrawCurve(g, mBuffer1, IColor(255, 100, 180, 255)); // corps 1 : bleu
    DrawCurve(g, mBuffer2, IColor(255, 255, 130, 80));  // corps 2 : corail
    DrawCurve(g, mBuffer3, IColor(255, 130, 220, 130)); // corps 3 : vert
  }

private:
  void DrawCurve(iplug::igraphics::IGraphics& g, const float* buf, const iplug::igraphics::IColor& color)
  {
    float midY = mRECT.MH();
    float w = mRECT.W();
    float h = mRECT.H() * 0.45f;

    for (int i = 0; i < mSize - 1; i++)
    {
      float x0 = mRECT.L + w * (float)i / (float)(mSize - 1);
      float x1 = mRECT.L + w * (float)(i + 1) / (float)(mSize - 1);
      float y0 = midY - buf[i] * h;
      float y1 = midY - buf[i + 1] * h;
      g.DrawLine(color, x0, y0, x1, y1, nullptr, 1.5f);
    }
  }

  static constexpr int kMaxPoints = 4096;
  float mBuffer1[kMaxPoints] = { 0.f };
  float mBuffer2[kMaxPoints] = { 0.f };
  float mBuffer3[kMaxPoints] = { 0.f };
  int mSize = 0;
};
