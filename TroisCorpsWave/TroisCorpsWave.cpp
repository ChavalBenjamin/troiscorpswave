#include "TroisCorpsWave.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

TroisCorpsWave::TroisCorpsWave(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kParamMass1)->InitDouble("Mass 1", 1., 0.05, 30., 0.01);
  GetParam(kParamMass2)->InitDouble("Mass 2", 1., 0.05, 30., 0.01);
  GetParam(kParamMass3)->InitDouble("Mass 3", 1., 0.05, 30., 0.01);
  GetParam(kParamRadius1)->InitDouble("Radius 1", 1.5, 0.1, 6., 0.01);
  GetParam(kParamRadius2)->InitDouble("Radius 2", 1.5, 0.1, 6., 0.01);
  GetParam(kParamRadius3)->InitDouble("Radius 3", 1.5, 0.1, 6., 0.01);
  GetParam(kParamAngle1)->InitDouble("Angle 1", 0., 0., 360., 0.1, "deg");
  GetParam(kParamAngle2)->InitDouble("Angle 2", 120., 0., 360., 0.1, "deg");
  GetParam(kParamAngle3)->InitDouble("Angle 3", 240., 0., 360., 0.1, "deg");
  GetParam(kParamOrbitalVelocity)->InitDouble("Orbital Vel", 0., 0., 5., 0.01);
  GetParam(kParamBoxSize)->InitDouble("Box Size", 1., 0.01, 2., 0.001);
  GetParam(kParamCaptureWindow)->InitDouble("Capture Window", 3., 1., 10., 0.001, "s");
  GetParam(kParamTableSize)->InitEnum("Table Size", 4, 7, "", IParam::kFlagsNone, "",
                                       "64", "128", "256", "512", "1024", "2048", "4096");
  GetParam(kParamBitDepth)->InitInt("Bit Depth", 16, 2, 16);

  GetParam(kParamVol1)->InitPercentage("Vol 1", 60.);
  GetParam(kParamVol2)->InitPercentage("Vol 2", 60.);
  GetParam(kParamVol3)->InitPercentage("Vol 3", 60.);

  GetParam(kParamAttack)->InitDouble("Attack", 0.01, 0.001, 2., 0.001, "s");
  GetParam(kParamDecay)->InitDouble("Decay", 0.3, 0.001, 3., 0.001, "s");
  GetParam(kParamSustain)->InitPercentage("Sustain", 70.);
  GetParam(kParamRelease)->InitDouble("Release", 0.3, 0.001, 5., 0.001, "s");

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
    constexpr float kControlsHeight = 530.f;
    IRECT controlsZone = bounds.GetFromTop(kControlsHeight).GetPadded(-20.f);

    constexpr int kRows = 6, kCols = 4;
    auto Cell = [&](int row, int col) { return controlsZone.GetGridCell(row, col, kRows, kCols); };

    pGraphics->AttachControl(new IVKnobControl(Cell(0, 0).GetCentredInside(48.f), kParamMass1, "Mass 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 1).GetCentredInside(48.f), kParamMass2, "Mass 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 2).GetCentredInside(48.f), kParamMass3, "Mass 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 3).GetCentredInside(48.f), kParamOrbitalVelocity, "Orbital Vel"));

    pGraphics->AttachControl(new IVKnobControl(Cell(1, 0).GetCentredInside(48.f), kParamRadius1, "Radius 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 1).GetCentredInside(48.f), kParamRadius2, "Radius 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 2).GetCentredInside(48.f), kParamRadius3, "Radius 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 3).GetCentredInside(48.f), kParamBoxSize, "Box Size"));

    pGraphics->AttachControl(new IVKnobControl(Cell(2, 0).GetCentredInside(48.f), kParamAngle1, "Angle 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(2, 1).GetCentredInside(48.f), kParamAngle2, "Angle 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(2, 2).GetCentredInside(48.f), kParamAngle3, "Angle 3"));

    pGraphics->AttachControl(new IVKnobControl(Cell(3, 0).GetCentredInside(48.f), kParamCaptureWindow, "Window"));
    pGraphics->AttachControl(new IVMenuButtonControl(Cell(3, 1).GetCentredInside(85.f, 28.f), kParamTableSize, "Table Size"));
    pGraphics->AttachControl(new IVKnobControl(Cell(3, 2).GetCentredInside(48.f), kParamBitDepth, "Bit Depth"));
    pGraphics->AttachControl(new IVButtonControl(Cell(3, 3).GetCentredInside(85.f, 28.f),
      [this](IControl* pCaller) { RequestCapture(); }, "Capture"));

    pGraphics->AttachControl(new IVKnobControl(Cell(4, 0).GetCentredInside(48.f), kParamVol1, "Vol 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(4, 1).GetCentredInside(48.f), kParamVol2, "Vol 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(4, 2).GetCentredInside(48.f), kParamVol3, "Vol 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(4, 3).GetCentredInside(48.f), kParamAttack, "Attack"));

    pGraphics->AttachControl(new IVKnobControl(Cell(5, 0).GetCentredInside(48.f), kParamDecay, "Decay"));
    pGraphics->AttachControl(new IVKnobControl(Cell(5, 1).GetCentredInside(48.f), kParamSustain, "Sustain"));
    pGraphics->AttachControl(new IVKnobControl(Cell(5, 2).GetCentredInside(48.f), kParamRelease, "Release"));

    // Apercu de la table capturee (corps 1), sur l'espace restant en bas
    IRECT waveArea = bounds.GetFromBottom(bounds.H() - kControlsHeight).GetPadded(-15.f);
    mWaveformView = new WaveformPreviewControl(waveArea);
    pGraphics->AttachControl(mWaveformView);
  };
#endif

#if IPLUG_DSP
  OnReset();
#endif
}

void TroisCorpsWave::OnIdle()
{
  if (mTableUpdatedForUI.exchange(false) && mWaveformView)
  {
    mWaveformView->SetWaveforms(mUITableCopy1, mUITableCopy2, mUITableCopy3, mUITableSize);
    mWaveformView->SetDirty(false);
  }
}

#if IPLUG_DSP

void TroisCorpsWave::DoCapture()
{
  double m1 = GetParam(kParamMass1)->Value();
  double m2 = GetParam(kParamMass2)->Value();
  double m3 = GetParam(kParamMass3)->Value();
  double r1 = GetParam(kParamRadius1)->Value();
  double r2 = GetParam(kParamRadius2)->Value();
  double r3 = GetParam(kParamRadius3)->Value();
  double a1 = GetParam(kParamAngle1)->Value();
  double a2 = GetParam(kParamAngle2)->Value();
  double a3 = GetParam(kParamAngle3)->Value();
  double orbitalVel = GetParam(kParamOrbitalVelocity)->Value();
  double boxSize = GetParam(kParamBoxSize)->Value();
  double captureWindow = GetParam(kParamCaptureWindow)->Value();

  mEngine.SetMasses(m1, m2, m3);
  mEngine.SetBoxSize(boxSize);
  mEngine.ResetBodies(r1, r2, r3, a1, a2, a3, orbitalVel);

  int nCaptured = mEngine.CaptureAllBodiesX(captureWindow, mRawCapture1, mRawCapture2, mRawCapture3, kMaxRawCapture);

  int tableSizeIdx = (int)GetParam(kParamTableSize)->Value();
  int tableSize = 64 << tableSizeIdx;

  if (nCaptured > 1)
  {
    mOsc1.SetTable(mRawCapture1, nCaptured, tableSize);
    mOsc2.SetTable(mRawCapture2, nCaptured, tableSize);
    mOsc3.SetTable(mRawCapture3, nCaptured, tableSize);

    // Apercu visuel des 3 corps, superposes
    mUITableSize = mOsc1.GetTableSize();
    const float* t1 = mOsc1.GetTable();
    const float* t2 = mOsc2.GetTable();
    const float* t3 = mOsc3.GetTable();
    for (int i = 0; i < mUITableSize; i++)
    {
      mUITableCopy1[i] = t1[i];
      mUITableCopy2[i] = t2[i];
      mUITableCopy3[i] = t3[i];
    }
    mTableUpdatedForUI.store(true);
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
  mOsc1.SetBitDepth((int)GetParam(kParamBitDepth)->Value());
  mOsc2.SetBitDepth((int)GetParam(kParamBitDepth)->Value());
  mOsc3.SetBitDepth((int)GetParam(kParamBitDepth)->Value());

  mEnv.SetSampleRate(GetSampleRate());
  UpdateEnvelopeParams();

  DoCapture(); // capture initiale au chargement
}

void TroisCorpsWave::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kParamMass1: case kParamMass2: case kParamMass3:
    case kParamRadius1: case kParamRadius2: case kParamRadius3:
    case kParamAngle1: case kParamAngle2: case kParamAngle3:
    case kParamOrbitalVelocity:
    case kParamBoxSize:
    case kParamCaptureWindow:
    case kParamTableSize:
      // On ne capture pas immediatement : on repousse un compte a rebours.
      // Si d'autres parametres changent juste apres (bouton tourne en
      // continu, ou plusieurs boutons a la fois), ce compte est repousse
      // a nouveau - une seule capture aura lieu, une fois que tout s'est
      // stabilise (voir ProcessBlock).
      mCaptureDebounceSamples = (int)(kDebounceMs * 0.001 * GetSampleRate());
      break;

    case kParamBitDepth:
    {
      int bits = (int)GetParam(kParamBitDepth)->Value();
      mOsc1.SetBitDepth(bits);
      mOsc2.SetBitDepth(bits);
      mOsc3.SetBitDepth(bits);
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
  if (mCaptureRequested.exchange(false))
  {
    DoCapture();
    mCaptureDebounceSamples = 0; // annule un delai en attente si Capture manuel presse
  }
  else if (mCaptureDebounceSamples > 0)
  {
    mCaptureDebounceSamples -= nFrames;
    if (mCaptureDebounceSamples <= 0)
    {
      mCaptureDebounceSamples = 0;
      DoCapture();
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
