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
