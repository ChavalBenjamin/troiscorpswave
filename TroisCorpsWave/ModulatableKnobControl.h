#pragma once

#include "IControls.h"

// ============================================================================
// ModulatableKnobControl / ModulatableSliderControl
//
// Variantes de IVKnobControl / IVSliderControl qui, lorsqu'une modulation
// est active, affichent (position ET valeur numerique) le resultat module
// en temps reel - sans jamais ecrire dans le vrai parametre de base (qui
// reste la reference manuelle de l'utilisateur, jamais alteree). Le
// controle devient non-interactif tant qu'il est module (logique : pas de
// sens a vouloir le tourner/glisser a la souris pendant qu'il est pilote
// ailleurs). Des que la modulation est desactivee (Aucun), le controle
// redevient parfaitement normal et interactif.
//
// NOTE TECHNIQUE : GetValue() n'etant pas virtuelle dans la classe de base
// (impossible a redefinir directement), on passe par Draw() (elle,
// virtuelle) : on echange temporairement la valeur juste le temps du
// dessin herite, puis on la restaure aussitot apres - le parametre reel
// n'est donc jamais durablement modifie par cet affichage.
// ============================================================================

class ModulatableKnobControl : public iplug::igraphics::IVKnobControl
{
public:
  using IVKnobControl::IVKnobControl;

  void SetModulation(float value0to1, bool active)
  {
    mModValue = value0to1;
    mModActive = active;
    SetDisabled(active);
    SetDirty(false);
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    if (mModActive)
    {
      double realValue = GetValue();
      SetValue(mModValue);
      IVKnobControl::Draw(g);
      SetValue(realValue);
    }
    else
    {
      IVKnobControl::Draw(g);
    }
  }

private:
  float mModValue = 0.f;
  bool mModActive = false;
};

class ModulatableSliderControl : public iplug::igraphics::IVSliderControl
{
public:
  using IVSliderControl::IVSliderControl;

  void SetModulation(float value0to1, bool active)
  {
    mModValue = value0to1;
    mModActive = active;
    SetDisabled(active);
    SetDirty(false);
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    if (mModActive)
    {
      double realValue = GetValue();
      SetValue(mModValue);
      IVSliderControl::Draw(g);
      SetValue(realValue);
    }
    else
    {
      IVSliderControl::Draw(g);
    }
  }

private:
  float mModValue = 0.f;
  bool mModActive = false;
};

// ============================================================================
// BodySourceSelector
//
// Selecteur compact a 4 segments (Aucun / Corps 1 / Corps 2 / Corps 3),
// entierement dessine et gere a la main (clic inclus), pour garantir que la
// couleur du segment selectionne corresponde bien au corps choisi (gris
// pour Aucun, bleu/corail/vert pour 1/2/3) - evite toute dependance a une
// convention de couleur interne d'IVTabSwitchControl qui s'est averee peu
// fiable.
// ============================================================================

class BodySourceSelector : public iplug::igraphics::IControl
{
public:
  BodySourceSelector(const iplug::igraphics::IRECT& bounds, int paramIdx)
  : IControl(bounds, paramIdx)
  {
  }

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    using namespace iplug::igraphics;

    int sel = (int)std::round(GetValue() * 3.0); // 0=Aucun, 1/2/3 = corps

    static const char* kLabels[4] = { "-", "1", "2", "3" };
    static const IColor kColors[4] = {
      IColor(255, 70, 70, 75),
      IColor(255, 100, 180, 255),
      IColor(255, 255, 130, 80),
      IColor(255, 130, 220, 130)
    };

    float segW = mRECT.W() / 4.f;
    IText labelText(11.f, COLOR_WHITE);

    for (int i = 0; i < 4; i++)
    {
      IRECT seg(mRECT.L + segW * (float)i, mRECT.T, mRECT.L + segW * (float)(i + 1), mRECT.B);
      bool active = (i == sel);

      g.FillRect(active ? kColors[i] : IColor(255, 35, 35, 40), seg);
      g.DrawRect(IColor(255, 20, 20, 25), seg);
      g.DrawText(labelText, kLabels[i], seg);
    }
  }

  void OnMouseDown(float x, float y, const iplug::igraphics::IMouseMod& mod) override
  {
    float segW = mRECT.W() / 4.f;
    int seg = (int)((x - mRECT.L) / segW);
    seg = std::clamp(seg, 0, 3);
    SetValue((double)seg / 3.0);
    SetDirty(true);
  }
};
