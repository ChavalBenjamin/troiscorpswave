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
// BodyColorTabSwitch
//
// Variante de IVTabSwitchControl dont la couleur de surbrillance change
// selon le segment actuellement selectionne (Aucun = gris, 1/2/3 = couleur
// du corps correspondant, coherente avec les graphiques). Suppose 4
// segments (Aucun/1/2/3).
// ============================================================================

class BodyColorTabSwitch : public iplug::igraphics::IVTabSwitchControl
{
public:
  using IVTabSwitchControl::IVTabSwitchControl;

  void Draw(iplug::igraphics::IGraphics& g) override
  {
    using namespace iplug::igraphics;

    int sel = (int)std::round(GetValue() * 3.0); // 0=Aucun, 1/2/3 = corps

    IColor highlight = COLOR_WHITE;
    if (sel == 1) highlight = IColor(255, 100, 180, 255); // corps 1 : bleu
    else if (sel == 2) highlight = IColor(255, 255, 130, 80); // corps 2 : corail
    else if (sel == 3) highlight = IColor(255, 130, 220, 130); // corps 3 : vert

    SetColor(kX1, highlight);
    IVTabSwitchControl::Draw(g);
  }
};
