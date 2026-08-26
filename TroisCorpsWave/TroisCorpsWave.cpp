#include "TroisCorpsWave.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include <cstdio>

TroisCorpsWave::TroisCorpsWave(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, 1))
{
  char nameBuf[32];

  for (int b = 0; b < 3; b++)
  {
    auto Name = [&](const char* base) -> const char* {
      std::snprintf(nameBuf, sizeof(nameBuf), "B%d %s", b + 1, base);
      return nameBuf;
    };

    GetParam(BankParam(b, kOffMass1))->InitDouble(Name("Mass1"), 1., 0.05, 30., 0.01);
    GetParam(BankParam(b, kOffMass2))->InitDouble(Name("Mass2"), 1., 0.05, 30., 0.01);
    GetParam(BankParam(b, kOffMass3))->InitDouble(Name("Mass3"), 1., 0.05, 30., 0.01);
    GetParam(BankParam(b, kOffRadius1))->InitDouble(Name("Radius1"), 1.5, 0.1, 6., 0.01);
    GetParam(BankParam(b, kOffRadius2))->InitDouble(Name("Radius2"), 1.5, 0.1, 6., 0.01);
    GetParam(BankParam(b, kOffRadius3))->InitDouble(Name("Radius3"), 1.5, 0.1, 6., 0.01);
    GetParam(BankParam(b, kOffAngle1))->InitDouble(Name("Angle1"), 0., 0., 360., 0.1, "deg");
    GetParam(BankParam(b, kOffAngle2))->InitDouble(Name("Angle2"), 120., 0., 360., 0.1, "deg");
    GetParam(BankParam(b, kOffAngle3))->InitDouble(Name("Angle3"), 240., 0., 360., 0.1, "deg");
    GetParam(BankParam(b, kOffOrbitalVel))->InitDouble(Name("OrbVel"), 0., 0., 5., 0.01);
    GetParam(BankParam(b, kOffBoxSize))->InitDouble(Name("Box"), 1., 0.01, 2., 0.001);
    GetParam(BankParam(b, kOffCaptureWindow))->InitDouble(Name("Window"), 3., 1., 10., 0.001, "s");
    GetParam(BankParam(b, kOffTableSize))->InitEnum(Name("TblSz"), 4, 7, "", IParam::kFlagsNone, "",
                                                      "64", "128", "256", "512", "1024", "2048", "4096");
  }

  GetParam(kParamVol1)->InitPercentage("Vol 1", 60.);
  GetParam(kParamVol2)->InitPercentage("Vol 2", 60.);
  GetParam(kParamVol3)->InitPercentage("Vol 3", 60.);
  GetParam(kParamAttack)->InitDouble("Attack", 0.01, 0.001, 2., 0.001, "s");
  GetParam(kParamDecay)->InitDouble("Decay", 0.3, 0.001, 3., 0.001, "s");
  GetParam(kParamSustain)->InitPercentage("Sustain", 70.);
  GetParam(kParamRelease)->InitDouble("Release", 0.3, 0.001, 5., 0.001, "s");
  GetParam(kParamBitDepth)->InitInt("Bit Depth", 16, 2, 16);
  GetParam(kParamMorph)->InitDouble("Morph", 0., 0., 127., 0.1);
  GetParam(kParamActiveTab)->InitEnum("Active Tab", 0, 3, "", IParam::kFlagsNone, "", "Bank 1", "Bank 2", "Bank 3");
  GetParam(kParamLFOBank)->InitEnum("LFO Bank", 0, 3, "", IParam::kFlagsNone, "", "1", "2", "3");
  GetParam(kParamCC1Number)->InitInt("CC 1", 20, 0, 127);
  GetParam(kParamCC2Number)->InitInt("CC 2", 21, 0, 127);
  GetParam(kParamCC3Number)->InitInt("CC 3", 22, 0, 127);
  GetParam(kParamLFORate)->InitDouble("LFO Rate", 0.1, 0.001, 1., 0.001);

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                         GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT bounds = pGraphics->GetBounds();
    float y = 0.f;
    auto NextSection = [&](float h) {
      IRECT r(bounds.L, bounds.T + y, bounds.R, bounds.T + y + h);
      y += h;
      return r;
    };

    // --- Panneau de fond distinct pour toute la zone "banque" (onglets +
    // --- grille + apercu), pour bien la separer visuellement de la zone
    // --- "parametres generaux" plus bas.
    IRECT bankPanel(bounds.L, bounds.T, bounds.R, bounds.T + 420.f);
    pGraphics->AttachControl(new IPanelControl(bankPanel, IColor(255, 45, 45, 50)));

    // --- Rangee d'onglets (Bank 1/2/3), avec surbrillance de l'onglet actif ---
    IRECT tabRow = NextSection(50.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVTabSwitchControl(tabRow, kParamActiveTab,
      { "Bank 1", "Bank 2", "Bank 3" }));

    // --- Grille des 9 parametres par corps (Mass/Radius/Angle x 3 corps),
    // --- alignee en colonnes par numero de corps (1, 2, 3) - plus la
    // --- colonne laterale des 4 parametres generaux de la banque
    // --- (Orbital Vel, Box, Window, Table Size), separee visuellement.
    IRECT bankGrid = NextSection(260.f).GetPadded(-10.f);
    IRECT mainGrid(bankGrid.L, bankGrid.T, bankGrid.L + bankGrid.W() * 0.72f, bankGrid.B);
    IRECT sideCol(mainGrid.R, bankGrid.T, bankGrid.R, bankGrid.B);

    const char* rowPrefix[3] = { "Mass", "Radius", "Angle" };
    int rowOffsets[3][3] = {
      { kOffMass1, kOffMass2, kOffMass3 },
      { kOffRadius1, kOffRadius2, kOffRadius3 },
      { kOffAngle1, kOffAngle2, kOffAngle3 }
    };

    int sideOffsets[4] = { kOffOrbitalVel, kOffBoxSize, kOffCaptureWindow, kOffTableSize };
    const char* sideLabels[4] = { "OrbVel", "Box", "Window", "TblSz" };

    IRECT bankWaveArea = NextSection(110.f).GetPadded(-10.f);

    for (int b = 0; b < 3; b++)
    {
      char label[16];

      for (int row = 0; row < 3; row++)
      {
        for (int col = 0; col < 3; col++)
        {
          int offset = rowOffsets[row][col];
          int paramIdx = BankParam(b, offset);
          IRECT cell = mainGrid.GetGridCell(row, col, 3, 3).GetCentredInside(46.f);
          std::snprintf(label, sizeof(label), "%s%d", rowPrefix[row], col + 1);

          IControl* ctrl = new IVKnobControl(cell, paramIdx, label);
          pGraphics->AttachControl(ctrl);
          ctrl->Hide(b != 0);
          mBankControls[b][offset] = ctrl;
        }
      }

      for (int s = 0; s < 4; s++)
      {
        int offset = sideOffsets[s];
        int paramIdx = BankParam(b, offset);
        IRECT cell = sideCol.GetGridCell(s, 0, 4, 1);

        IControl* ctrl;
        if (offset == kOffTableSize)
          ctrl = new IVMenuButtonControl(cell.GetCentredInside(75.f, 24.f), paramIdx, sideLabels[s]);
        else
          ctrl = new IVKnobControl(cell.GetCentredInside(40.f), paramIdx, sideLabels[s]);

        pGraphics->AttachControl(ctrl);
        ctrl->Hide(b != 0);
        mBankControls[b][offset] = ctrl;
      }

      auto* waveView = new WaveformPreviewControl(bankWaveArea);
      pGraphics->AttachControl(waveView);
      waveView->Hide(b != 0);
      mBankWaveView[b] = waveView;
    }

    // --- Parametres partages : Vol 1/2/3 + Bit Depth ---
    // --- Panneau de fond distinct pour la zone "parametres generaux" ---
    IRECT generalPanel(bounds.L, bounds.T + y, bounds.R, bounds.T + y + 130.f);
    pGraphics->AttachControl(new IPanelControl(generalPanel, IColor(255, 55, 55, 60)));

    IRECT sharedRow1 = NextSection(65.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVKnobControl(sharedRow1.GetGridCell(0, 0, 1, 4).GetCentredInside(46.f), kParamVol1, "Vol 1"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow1.GetGridCell(0, 1, 1, 4).GetCentredInside(46.f), kParamVol2, "Vol 2"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow1.GetGridCell(0, 2, 1, 4).GetCentredInside(46.f), kParamVol3, "Vol 3"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow1.GetGridCell(0, 3, 1, 4).GetCentredInside(46.f), kParamBitDepth, "BitDepth"));

    // --- Parametres partages : ADSR ---
    IRECT sharedRow2 = NextSection(65.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVKnobControl(sharedRow2.GetGridCell(0, 0, 1, 4).GetCentredInside(46.f), kParamAttack, "Attack"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow2.GetGridCell(0, 1, 1, 4).GetCentredInside(46.f), kParamDecay, "Decay"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow2.GetGridCell(0, 2, 1, 4).GetCentredInside(46.f), kParamSustain, "Sustain"));
    pGraphics->AttachControl(new IVKnobControl(sharedRow2.GetGridCell(0, 3, 1, 4).GetCentredInside(46.f), kParamRelease, "Release"));

    // --- Zone du bas, coupee en deux colonnes : Morph (gauche) / LFO+Anime (droite) ---
    IRECT lowerArea(bounds.L, bounds.T + y, bounds.R, bounds.B);
    IRECT morphCol(lowerArea.L, lowerArea.T, lowerArea.L + lowerArea.W() * 0.6f, lowerArea.B);
    IRECT lfoCol(lowerArea.L + lowerArea.W() * 0.6f, lowerArea.T, lowerArea.R, lowerArea.B);

    // Colonne gauche : slider de morphing (0 = banque1, 63 = banque2, 127 = banque3)
    // puis apercu du resultat du morphing en direct.
    IRECT morphRow = morphCol.GetFromTop(55.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVSliderControl(morphRow, kParamMorph, "Morph", DEFAULT_STYLE, true, EDirection::Horizontal));

    IRECT morphWaveArea = IRECT(morphCol.L, morphCol.T + 55.f, morphCol.R, morphCol.B).GetPadded(-10.f);
    mMorphWaveView = new WaveformPreviewControl(morphWaveArea);
    pGraphics->AttachControl(mMorphWaveView);

    // Colonne droite : selecteur de banque LFO (1/2/3, independant des
    // onglets d'edition), reglage de vitesse, puis animation des 3 corps.
    IRECT lfoTabRow = lfoCol.GetFromTop(50.f).GetPadded(-8.f);
    pGraphics->AttachControl(new IVTabSwitchControl(lfoTabRow, kParamLFOBank, { "1", "2", "3" }));

    IRECT lfoRateRow = IRECT(lfoCol.L, lfoCol.T + 50.f, lfoCol.R, lfoCol.T + 110.f).GetPadded(-8.f);
    pGraphics->AttachControl(new IVKnobControl(lfoRateRow.GetCentredInside(46.f), kParamLFORate, "LFO Rate"));

    IRECT animArea = IRECT(lfoCol.L, lfoCol.T + 110.f, lfoCol.R, lfoCol.B).GetPadded(-10.f);
    mAnimView = new BodyAnimationControl(animArea);
    pGraphics->AttachControl(mAnimView);
  };
#endif

#if IPLUG_DSP
  OnReset();
#endif
}

void TroisCorpsWave::SwitchTab(int tabIdx)
{
  if (tabIdx < 0 || tabIdx > 2) return;
  mActiveTab = tabIdx;

  for (int b = 0; b < 3; b++)
  {
    bool visible = (b == tabIdx);
    for (int k = 0; k < kNumBankParams; k++)
      if (mBankControls[b][k]) mBankControls[b][k]->Hide(!visible);
    if (mBankWaveView[b]) mBankWaveView[b]->Hide(!visible);
  }

  // Rafraichit explicitement le graphique de l'onglet qu'on vient
  // d'afficher, avec les dernieres donnees connues de cette banque -
  // sans ca, le passage d'un onglet a l'autre pouvait laisser un
  // graphique vide/perime (les donnees etaient bien calculees, mais
  // jamais repoussees vers l'affichage au moment precis du clic).
  if (mBankWaveView[tabIdx])
  {
    mBankWaveView[tabIdx]->SetWaveforms(mBankUICopy1[tabIdx], mBankUICopy2[tabIdx], mBankUICopy3[tabIdx], mBankUISize[tabIdx]);
    mBankWaveView[tabIdx]->SetDirty(true);
  }
}

void TroisCorpsWave::OnIdle()
{
  bool anyBankJustUpdated = false;

  for (int b = 0; b < 3; b++)
  {
    if (mBankUIUpdated[b].exchange(false) && mBankWaveView[b])
    {
      mBankWaveView[b]->SetWaveforms(mBankUICopy1[b], mBankUICopy2[b], mBankUICopy3[b], mBankUISize[b]);
      anyBankJustUpdated = true;
    }
    // Toujours redessiner (peu couteux), meme si les donnees n'ont pas
    // change depuis la derniere fois - evite un graphique fige/vide apres
    // que la fenetre du plugin ait ete masquee puis reaffichee (ex.
    // changement d'application puis retour sur Reaper).
    if (mBankWaveView[b]) mBankWaveView[b]->SetDirty(false);
  }

  if (mMorphWaveView)
  {
    float morph = mUIMorphPosition.load();

    if (morph != mLastDrawnMorph || anyBankJustUpdated)
    {
      mLastDrawnMorph = morph;

      constexpr int kPreviewRes = 512;
      static float blend1[kPreviewRes], blend2[kPreviewRes], blend3[kPreviewRes];

      auto SampleBank = [&](const float* buf, int size, int idx) -> float {
        if (size <= 0) return 0.f;
        float pos = (float)idx / (float)kPreviewRes * (float)size;
        int i0 = (int)pos % size;
        int i1 = (i0 + 1) % size;
        float frac = pos - (float)(int)pos;
        return buf[i0] * (1.f - frac) + buf[i1] * frac;
      };

      auto Morph3 = [&](float a, float b, float c) -> float {
        if (morph <= 63.f) { float t = morph / 63.f; return a * (1.f - t) + b * t; }
        float t = (morph - 63.f) / 64.f;
        return b * (1.f - t) + c * t;
      };

      for (int i = 0; i < kPreviewRes; i++)
      {
        float s0_1 = SampleBank(mBankUICopy1[0], mBankUISize[0], i);
        float s1_1 = SampleBank(mBankUICopy1[1], mBankUISize[1], i);
        float s2_1 = SampleBank(mBankUICopy1[2], mBankUISize[2], i);
        float s0_2 = SampleBank(mBankUICopy2[0], mBankUISize[0], i);
        float s1_2 = SampleBank(mBankUICopy2[1], mBankUISize[1], i);
        float s2_2 = SampleBank(mBankUICopy2[2], mBankUISize[2], i);
        float s0_3 = SampleBank(mBankUICopy3[0], mBankUISize[0], i);
        float s1_3 = SampleBank(mBankUICopy3[1], mBankUISize[1], i);
        float s2_3 = SampleBank(mBankUICopy3[2], mBankUISize[2], i);

        blend1[i] = Morph3(s0_1, s1_1, s2_1);
        blend2[i] = Morph3(s0_2, s1_2, s2_2);
        blend3[i] = Morph3(s0_3, s1_3, s2_3);
      }

      mMorphWaveView->SetWaveforms(blend1, blend2, blend3, kPreviewRes);
    }
    mMorphWaveView->SetDirty(false); // toujours, meme sans recalcul (voir plus haut)
  }

  // Anime les 3 corps de la banque LFO selectionnee, en continu.
  if (mAnimView)
  {
    mAnimView->SetPositions(
      mUIBodyX[0].load(), mUIBodyY[0].load(),
      mUIBodyX[1].load(), mUIBodyY[1].load(),
      mUIBodyX[2].load(), mUIBodyY[2].load(),
      mUIAnimScale.load());
    mAnimView->SetDirty(false);
  }
}

#if IPLUG_DSP

void TroisCorpsWave::DoCaptureBank(int bankIdx)
{
  double m1 = GetParam(BankParam(bankIdx, kOffMass1))->Value();
  double m2 = GetParam(BankParam(bankIdx, kOffMass2))->Value();
  double m3 = GetParam(BankParam(bankIdx, kOffMass3))->Value();
  double r1 = GetParam(BankParam(bankIdx, kOffRadius1))->Value();
  double r2 = GetParam(BankParam(bankIdx, kOffRadius2))->Value();
  double r3 = GetParam(BankParam(bankIdx, kOffRadius3))->Value();
  double a1 = GetParam(BankParam(bankIdx, kOffAngle1))->Value();
  double a2 = GetParam(BankParam(bankIdx, kOffAngle2))->Value();
  double a3 = GetParam(BankParam(bankIdx, kOffAngle3))->Value();
  double orbitalVel = GetParam(BankParam(bankIdx, kOffOrbitalVel))->Value();
  double boxSize = GetParam(BankParam(bankIdx, kOffBoxSize))->Value();
  double captureWindow = GetParam(BankParam(bankIdx, kOffCaptureWindow))->Value();
  int tableSizeIdx = (int)GetParam(BankParam(bankIdx, kOffTableSize))->Value();
  int tableSize = 64 << tableSizeIdx;

  mEngine.SetMasses(m1, m2, m3);
  mEngine.SetBoxSize(boxSize);
  mEngine.ResetBodies(r1, r2, r3, a1, a2, a3, orbitalVel);

  int nCaptured = mEngine.CaptureAllBodiesXY(captureWindow, mRawX1, mRawY1, mRawX2, mRawY2, mRawX3, mRawY3, kMaxRawCapture);

  if (nCaptured > 1)
  {
    mOsc1.SetBankTable(bankIdx, mRawX1, nCaptured, tableSize);
    mOsc2.SetBankTable(bankIdx, mRawX2, nCaptured, tableSize);
    mOsc3.SetBankTable(bankIdx, mRawX3, nCaptured, tableSize);

    mBankUISize[bankIdx] = mOsc1.GetBankTableSize(bankIdx);
    const float* t1 = mOsc1.GetBankTable(bankIdx);
    const float* t2 = mOsc2.GetBankTable(bankIdx);
    const float* t3 = mOsc3.GetBankTable(bankIdx);
    for (int i = 0; i < mBankUISize[bankIdx]; i++)
    {
      mBankUICopy1[bankIdx][i] = t1[i];
      mBankUICopy2[bankIdx][i] = t2[i];
      mBankUICopy3[bankIdx][i] = t3[i];
    }
    mBankUIUpdated[bankIdx].store(true);

    // Reechantillonnage X,Y (les 3 corps) pour l'animation/LFO, a une
    // resolution fixe independante de la table audio, + calcul de
    // l'echelle d'affichage (etendue max toutes coordonnees confondues,
    // pour garder les proportions coherentes entre les 3 corps).
    auto ResampleXY = [&](const float* srcX, const float* srcY, float* dstX, float* dstY)
    {
      for (int i = 0; i < kAnimRes; i++)
      {
        float srcPos = (float)i / (float)kAnimRes * (float)nCaptured;
        int i0 = (int)srcPos;
        int i1 = std::min(i0 + 1, nCaptured - 1);
        float frac = srcPos - (float)i0;
        dstX[i] = srcX[i0] * (1.f - frac) + srcX[i1] * frac;
        dstY[i] = srcY[i0] * (1.f - frac) + srcY[i1] * frac;
      }
    };

    ResampleXY(mRawX1, mRawY1, mBankAnimX[bankIdx][0], mBankAnimY[bankIdx][0]);
    ResampleXY(mRawX2, mRawY2, mBankAnimX[bankIdx][1], mBankAnimY[bankIdx][1]);
    ResampleXY(mRawX3, mRawY3, mBankAnimX[bankIdx][2], mBankAnimY[bankIdx][2]);

    float maxExtent = 1e-6f;
    for (int body = 0; body < 3; body++)
    {
      for (int i = 0; i < kAnimRes; i++)
      {
        maxExtent = std::max(maxExtent, std::abs(mBankAnimX[bankIdx][body][i]));
        maxExtent = std::max(maxExtent, std::abs(mBankAnimY[bankIdx][body][i]));
      }
    }
    mBankAnimScale[bankIdx] = maxExtent;
  }
}

void TroisCorpsWave::UpdateEnvelopeParams()
{
  double attack = GetParam(kParamAttack)->Value();
  double decay = GetParam(kParamDecay)->Value();
  double sustain = GetParam(kParamSustain)->Value() / 100.0;
  double release = GetParam(kParamRelease)->Value();
  mEnv.SetADSR(attack, decay, sustain, release);
}

void TroisCorpsWave::OnReset()
{
  mOsc1.SetSampleRate(GetSampleRate());
  mOsc2.SetSampleRate(GetSampleRate());
  mOsc3.SetSampleRate(GetSampleRate());

  int bits = (int)GetParam(kParamBitDepth)->Value();
  mOsc1.SetBitDepth(bits);
  mOsc2.SetBitDepth(bits);
  mOsc3.SetBitDepth(bits);

  float morph = (float)GetParam(kParamMorph)->Value();
  mOsc1.SetMorphPosition(morph);
  mOsc2.SetMorphPosition(morph);
  mOsc3.SetMorphPosition(morph);
  mUIMorphPosition.store(morph);

  mEnv.SetSampleRate(GetSampleRate());
  UpdateEnvelopeParams();

  mLFOPhase = 0.0;
  mLastSentCC1 = mLastSentCC2 = mLastSentCC3 = -1;

  for (int b = 0; b < 3; b++)
    DoCaptureBank(b);
}

void TroisCorpsWave::OnParamChange(int paramIdx)
{
  for (int b = 0; b < 3; b++)
  {
    int start = kParamBank0Start + b * kNumBankParams;
    if (paramIdx >= start && paramIdx < start + kNumBankParams)
    {
      mBankDebounceSamples[b] = (int)(kDebounceMs * 0.001 * GetSampleRate());
      return;
    }
  }

  switch (paramIdx)
  {
    case kParamBitDepth:
    {
      int bits = (int)GetParam(kParamBitDepth)->Value();
      mOsc1.SetBitDepth(bits);
      mOsc2.SetBitDepth(bits);
      mOsc3.SetBitDepth(bits);
      break;
    }
    case kParamMorph:
    {
      float morph = (float)GetParam(kParamMorph)->Value();
      mOsc1.SetMorphPosition(morph);
      mOsc2.SetMorphPosition(morph);
      mOsc3.SetMorphPosition(morph);
      mUIMorphPosition.store(morph);
      break;
    }
    case kParamAttack:
    case kParamDecay:
    case kParamSustain:
    case kParamRelease:
      UpdateEnvelopeParams();
      break;
    case kParamActiveTab:
      SwitchTab((int)GetParam(kParamActiveTab)->Value());
      break;
    case kParamLFOBank:
      // Redemarre la boucle proprement au changement de banque source,
      // plutot que de sauter au milieu d'un cycle different.
      mLFOPhase = 0.0;
      break;
    default:
      break;
  }
}

void TroisCorpsWave::ProcessMidiMsg(const IMidiMsg& msg)
{
  if (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() > 0)
  {
    mOsc1.NoteOn(msg.NoteNumber());
    mOsc2.NoteOn(msg.NoteNumber());
    mOsc3.NoteOn(msg.NoteNumber());
    mEnv.NoteOn();
  }
  else if (msg.StatusMsg() == IMidiMsg::kNoteOff ||
           (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() == 0))
  {
    mEnv.NoteOff();
  }
}

void TroisCorpsWave::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  for (int b = 0; b < 3; b++)
  {
    if (mBankDebounceSamples[b] > 0)
    {
      mBankDebounceSamples[b] -= nFrames;
      if (mBankDebounceSamples[b] <= 0)
      {
        mBankDebounceSamples[b] = 0;
        DoCaptureBank(b);
      }
    }
  }

  float vol1 = (float)(GetParam(kParamVol1)->Value() / 100.0);
  float vol2 = (float)(GetParam(kParamVol2)->Value() / 100.0);
  float vol3 = (float)(GetParam(kParamVol3)->Value() / 100.0);

  int lfoBank = (int)GetParam(kParamLFOBank)->Value();
  double captureWindow = GetParam(BankParam(lfoBank, kOffCaptureWindow))->Value();
  double lfoRate = GetParam(kParamLFORate)->Value();
  double phaseInc = (1.0 / GetSampleRate()) / std::max(0.001, captureWindow) * lfoRate;
  int cc1Number = (int)GetParam(kParamCC1Number)->Value();
  int cc2Number = (int)GetParam(kParamCC2Number)->Value();
  int cc3Number = (int)GetParam(kParamCC3Number)->Value();
  float animScale = mBankAnimScale[lfoBank];

  for (int i = 0; i < nFrames; i++)
  {
    float envLevel = mEnv.Process();

    if (!mEnv.IsActive())
    {
      mOsc1.NoteOff();
      mOsc2.NoteOff();
      mOsc3.NoteOff();
    }

    float mix = (mOsc1.Process() * vol1 + mOsc2.Process() * vol2 + mOsc3.Process() * vol3) * envLevel;

    outputs[0][i] = mix;
    outputs[1][i] = mix;

    // --- LFO/CC + animation : boucle libre en continu, calee sur la
    // duree reelle de la fenetre de capture de la banque LFO selectionnee.
    float animPos = (float)mLFOPhase * (float)kAnimRes;
    int idx0 = (int)animPos % kAnimRes;
    int idx1 = (idx0 + 1) % kAnimRes;
    float frac = animPos - (float)(int)animPos;

    auto Interp = [&](const float* buf) { return buf[idx0] * (1.f - frac) + buf[idx1] * frac; };

    float x1 = Interp(mBankAnimX[lfoBank][0]);
    float y1 = Interp(mBankAnimY[lfoBank][0]);
    float x2 = Interp(mBankAnimX[lfoBank][1]);
    float y2 = Interp(mBankAnimY[lfoBank][1]);
    float x3 = Interp(mBankAnimX[lfoBank][2]);
    float y3 = Interp(mBankAnimY[lfoBank][2]);

    auto ToCC = [&](float x) { return (int)std::clamp(((x / animScale) + 1.f) * 0.5f * 127.f, 0.f, 127.f); };

    int cc1 = ToCC(x1);
    if (cc1 != mLastSentCC1)
    {
      IMidiMsg msg;
      msg.MakeControlChangeMsg((IMidiMsg::EControlChangeMsg)cc1Number, cc1 / 127.0, 0, i);
      SendMidiMsg(msg);
      mLastSentCC1 = cc1;
    }
    int cc2 = ToCC(x2);
    if (cc2 != mLastSentCC2)
    {
      IMidiMsg msg;
      msg.MakeControlChangeMsg((IMidiMsg::EControlChangeMsg)cc2Number, cc2 / 127.0, 0, i);
      SendMidiMsg(msg);
      mLastSentCC2 = cc2;
    }
    int cc3 = ToCC(x3);
    if (cc3 != mLastSentCC3)
    {
      IMidiMsg msg;
      msg.MakeControlChangeMsg((IMidiMsg::EControlChangeMsg)cc3Number, cc3 / 127.0, 0, i);
      SendMidiMsg(msg);
      mLastSentCC3 = cc3;
    }

    if (i == nFrames - 1) // suffit de mettre a jour l'affichage une fois par bloc
    {
      mUIBodyX[0].store(x1); mUIBodyY[0].store(y1);
      mUIBodyX[1].store(x2); mUIBodyY[1].store(y2);
      mUIBodyX[2].store(x3); mUIBodyY[2].store(y3);
      mUIAnimScale.store(animScale);
    }

    mLFOPhase += phaseInc;
    if (mLFOPhase >= 1.0) mLFOPhase -= 1.0;
  }
}

#endif
