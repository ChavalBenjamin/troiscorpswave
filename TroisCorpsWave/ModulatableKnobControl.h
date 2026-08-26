#pragma once

#include "IControls.h"

// ============================================================================
// ModulatableKnobControl / ModulatableSliderControl
//
// Variantes de IVKnobControl / IVSliderControl qui, lorsqu'une modulation
// est active, affichent (position ET valeur numerique) le resultat module
// en temps reel - SANS jamais ecrire dans le vrai parametre de base (qui
// reste la reference manuelle de l'utilisateur, jamais alteree). Le
// controle devient non-interactif tant qu'il est module (logique : pas de
// sens a vouloir le tourner/glisser a la souris pendant qu'il est pilote
// ailleurs). Des que la modulation est desactivee (Aucun), le controle
// redevient parfaitement normal et interactif.
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

  double GetValue(int valIdx = 0) const override
  {
    if (mModActive) return mModValue;
    return IVKnobControl::GetValue(valIdx);
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

  double GetValue(int valIdx = 0) const override
  {
    if (mModActive) return mModValue;
    return IVSliderControl::GetValue(valIdx);
  }

private:
  float mModValue = 0.f;
  bool mModActive = false;
};
