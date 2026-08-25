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
  GetParam(kParamOrbitalVelocity)->InitDouble("Orbital Vel", 0., 0., 5., 0.01);
  GetParam(kParamBoxSize)->InitDouble("Box Size", 1., 0.01, 2., 0.001);
  GetParam(kParamCaptureWindow)->InitDouble("Capture Window", 3., 1., 10., 0.001, "s");
  GetParam(kParamTableSize)->InitEnum("Table Size", 4, 7, "", IParam::kFlagsNone, "",
                                       "64", "128", "256", "512", "1024", "2048", "4096");
  GetParam(kParamBitDepth)->InitInt("Bit Depth", 16, 2, 16);

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
    IRECT controlsZone = bounds.GetFromTop(280.f).GetPadded(-20.f);
    IRECT area = controlsZone;

    constexpr int kRows = 3, kCols = 4;
    auto Cell = [&](int row, int col) { return area.GetGridCell(row, col, kRows, kCols); };

    pGraphics->AttachControl(new IVKnobControl(Cell(0, 0).GetCentredInside(50.f), kParamMass1, "Mass 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 1).GetCentredInside(50.f), kParamMass2, "Mass 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 2).GetCentredInside(50.f), kParamMass3, "Mass 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 3).GetCentredInside(50.f), kParamOrbitalVelocity, "Orbital Vel"));

    pGraphics->AttachControl(new IVKnobControl(Cell(1, 0).GetCentredInside(50.f), kParamRadius1, "Radius 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 1).GetCentredInside(50.f), kParamRadius2, "Radius 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 2).GetCentredInside(50.f), kParamRadius3, "Radius 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 3).GetCentredInside(50.f), kParamBoxSize, "Box Size"));

    pGraphics->AttachControl(new IVKnobControl(Cell(2, 0).GetCentredInside(50.f), kParamCaptureWindow, "Window"));
    pGraphics->AttachControl(new IVMenuButtonControl(Cell(2, 1).GetCentredInside(90.f, 30.f), kParamTableSize, "Table Size"));
    pGraphics->AttachControl(new IVKnobControl(Cell(2, 2).GetCentredInside(50.f), kParamBitDepth, "Bit Depth"));
    pGraphics->AttachControl(new IVButtonControl(Cell(2, 3).GetCentredInside(90.f, 30.f),
      [this](IControl* pCaller) { RequestCapture(); }, "Capture"));

    // Aperçu de la table capturee, sur l'espace restant en bas
    IRECT waveArea = bounds.GetFromBottom(bounds.H() - 280.f).GetPadded(-15.f);
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
    mWaveformView->SetWaveform(mUITableCopy, mUITableSize);
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
  double orbitalVel = GetParam(kParamOrbitalVelocity)->Value();
  double boxSize = GetParam(kParamBoxSize)->Value();
  double captureWindow = GetParam(kParamCaptureWindow)->Value();

  mEngine.SetMasses(m1, m2, m3);
  mEngine.SetBoxSize(boxSize);
  mEngine.ResetBodies(r1, r2, r3, orbitalVel);

  int nCaptured = mEngine.CaptureBody1X(captureWindow, mRawCaptureBuffer, kMaxRawCapture);

  int tableSizeIdx = (int)GetParam(kParamTableSize)->Value();
  int tableSize = 64 << tableSizeIdx; // 0->64, 1->128, ... 6->4096

  if (nCaptured > 1)
  {
    mOsc.SetTable(mRawCaptureBuffer, nCaptured, tableSize);

    mUITableSize = mOsc.GetTableSize();
    const float* finalTable = mOsc.GetTable();
    for (int i = 0; i < mUITableSize; i++)
      mUITableCopy[i] = finalTable[i];
    mTableUpdatedForUI.store(true);
  }
}

void TroisCorpsWave::OnReset()
{
  mOsc.SetSampleRate(GetSampleRate());
  mOsc.SetBitDepth((int)GetParam(kParamBitDepth)->Value());
  DoCapture(); // capture initiale au chargement, pour qu'il y ait deja un son avant meme d'appuyer sur Capture
}

void TroisCorpsWave::OnParamChange(int paramIdx)
{
  if (paramIdx == kParamBitDepth)
    mOsc.SetBitDepth((int)GetParam(kParamBitDepth)->Value());
}

void TroisCorpsWave::ProcessMidiMsg(const IMidiMsg& msg)
{
  if (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() > 0)
  {
    mOsc.NoteOn(msg.NoteNumber());
    mFadeTarget = 1.f;
  }
  else if (msg.StatusMsg() == IMidiMsg::kNoteOff ||
           (msg.StatusMsg() == IMidiMsg::kNoteOn && msg.Velocity() == 0))
  {
    mFadeTarget = 0.f;
  }
}

void TroisCorpsWave::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  if (mCaptureRequested.exchange(false))
    DoCapture();

  for (int i = 0; i < nFrames; i++)
  {
    if (mFadeGain < mFadeTarget) mFadeGain = std::min(mFadeGain + kFadeStep, mFadeTarget);
    else if (mFadeGain > mFadeTarget) mFadeGain = std::max(mFadeGain - kFadeStep, mFadeTarget);

    if (mFadeGain <= 0.f && mFadeTarget <= 0.f)
      mOsc.NoteOff();

    float sample = mOsc.Process() * mFadeGain;

    outputs[0][i] = sample;
    outputs[1][i] = sample;
  }
}

#endif
