#pragma once

#include "IControl.h"
#include <cmath>

// ============================================================================
// BodyAnimationControl
//
// Affiche les 3 corps sous forme de points en mouvement (animation 2D),
// pour la banque actuellement selectionnee dans le panneau LFO. Recoit ses
// positions via SetPositions() depuis le thread interface (OnIdle), jamais
// depuis l'audio.
// ============================================================================

class BodyAnimationControl : public iplug::igraphics::IControl
{
public:
  BodyAnimationControl(const iplug::igraphics::IRECT& bounds)
  : IControl(bounds)
  {
  }

  void SetPositions(double x1, double y1, double x2, double y2, double x3, double y3, double scale)
  {
    mX1 = x1; mY1 = y1;
    mX2 = x2; mY2 = y2;
    mX3 = x3; mY3 = y3;
    mScale = scale > 1e-6 ? scale : 1.0;
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    using namespace iplug::igraphics;

    g.FillRect(IColor(255, 15, 15, 20), mRECT);

    float cx = mRECT.MW();
    float cy = mRECT.MH();
    float radius = std::min(mRECT.W(), mRECT.H()) * 0.42f;

    auto Plot = [&](double x, double y, const IColor& color)
    {
      float px = cx + (float)(x / mScale) * radius;
      float py = cy - (float)(y / mScale) * radius;
      g.FillCircle(color, px, py, 6.f);
    };

    Plot(mX1, mY1, IColor(255, 100, 180, 255));  // corps 1 : bleu
    Plot(mX2, mY2, IColor(255, 255, 130, 80));   // corps 2 : corail
    Plot(mX3, mY3, IColor(255, 130, 220, 130));  // corps 3 : vert
  }

private:
  double mX1 = 0.0, mY1 = 0.0;
  double mX2 = 0.0, mY2 = 0.0;
  double mX3 = 0.0, mY3 = 0.0;
  double mScale = 1.0;
};
