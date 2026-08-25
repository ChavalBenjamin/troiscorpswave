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

    // --- Rangee d'onglets (Bank 1/2/3) + bouton Capture ---
    IRECT tabRow = NextSection(50.f);
    pGraphics->AttachControl(new IVButtonControl(tabRow.GetGridCell(0, 0, 1, 4).GetCentredInside(90.f, 32.f),
      [this](IControl* pCaller) { SwitchTab(0); }, "Bank 1"));
    pGraphics->AttachControl(new IVButtonControl(tabRow.GetGridCell(0, 1, 1, 4).GetCentredInside(90.f, 32.f),
      [this](IControl* pCaller) { SwitchTab(1); }, "Bank 2"));
    pGraphics->AttachControl(new IVButtonControl(tabRow.GetGridCell(0, 2, 1, 4).GetCentredInside(90.f, 32.f),
      [this](IControl* pCaller) { SwitchTab(2); }, "Bank 3"));
    pGraphics->AttachControl(new IVButtonControl(tabRow.GetGridCell(0, 3, 1, 4).GetCentredInside(90.f, 32.f),
      [this](IControl* pCaller) { RequestCaptureActiveTab(); }, "Capture"));

    // --- Grille de 13 controles physiques, dupliquee x3 (une par banque), ---
    // --- seule celle de l'onglet actif est visible au depart (Bank 1)    ---
    IRECT bankGrid = NextSection(260.f).GetPadded(-10.f);
    const char* bankLabels[kNumBankParams] = {
      "Mass1", "Mass2", "Mass3", "Radius1", "Radius2", "Radius3",
      "Angle1", "Angle2", "Angle3", "OrbVel", "Box", "Window", "TblSz"
    };
    int bankOrder[kNumBankParams] = {
      kOffMass1, kOffMass2, kOffMass3, kOffRadius1, kOffRadius2, kOffRadius3,
      kOffAngle1, kOffAngle2, kOffAngle3, kOffOrbitalVel, kOffBoxSize, kOffCaptureWindow, kOffTableSize
    };

    IRECT bankWaveArea = NextSection(110.f).GetPadded(-10.f);

    for (int b = 0; b < 3; b++)
    {
      for (int k = 0; k < kNumBankParams; k++)
      {
        int row = k / 4;
        int col = k % 4;
        int offset = bankOrder[k];
        int paramIdx = BankParam(b, offset);
        IRECT baseCell = bankGrid.GetGridCell(row, col, 4, 4);

        IControl* ctrl;
        if (offset == kOffTableSize)
          ctrl = new IVMenuButtonControl(baseCell.GetCentredInside(80.f, 26.f), paramIdx, bankLabels[k]);
        else
          ctrl = new IVKnobControl(baseCell.GetCentredInside(44.f), paramIdx, bankLabels[k]);

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

    // --- Slider de morphing (0 = banque1, 63 = banque2, 127 = banque3) ---
    IRECT morphRow = NextSection(55.f).GetPadded(-10.f);
    pGraphics->AttachControl(new IVSliderControl(morphRow, kParamMorph, "Morph", DEFAULT_STYLE, true, EDirection::Horizontal));

    // --- Apercu du resultat du morphing en direct, toujours visible ---
    IRECT morphWaveArea = IRECT(bounds.L, bounds.T + y, bounds.R, bounds.B).GetPadded(-10.f);
    mMorphWaveView = new WaveformPreviewControl(morphWaveArea);
    pGraphics->AttachControl(mMorphWaveView);
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
}

void TroisCorpsWave::OnIdle()
{
  for (int b = 0; b < 3; b++)
  {
    if (mBankUIUpdated[b].exchange(false) && mBankWaveView[b])
    {
      mBankWaveView[b]->SetWaveforms(mBankUICopy1[b], mBankUICopy2[b], mBankUICopy3[b], mBankUISize[b]);
      mBankWaveView[b]->SetDirty(false);
    }
  }

  if (mMorphWaveView)
  {
    float morph = mUIMorphPosition.load();
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
    mMorphWaveView->SetDirty(false);
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

  int nCaptured = mEngine.CaptureAllBodiesX(captureWindow, mRawCapture1, mRawCapture2, mRawCapture3, kMaxRawCapture);

  if (nCaptured > 1)
  {
    mOsc1.SetBankTable(bankIdx, mRawCapture1, nCaptured, tableSize);
    mOsc2.SetBankTable(bankIdx, mRawCapture2, nCaptured, tableSize);
    mOsc3.SetBankTable(bankIdx, mRawCapture3, nCaptured, tableSize);

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
  int forced = mForceCaptureBank.exchange(-1);
  if (forced >= 0 && forced < 3)
  {
    DoCaptureBank(forced);
    mBankDebounceSamples[forced] = 0;
  }

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
  }
}

#endif
