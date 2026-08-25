#include "TroisCorpsWave.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

TroisCorpsWave::TroisCorpsWave(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kParamMass1)->InitDouble("Mass 1", 1., 0.1, 10., 0.01);
  GetParam(kParamMass2)->InitDouble("Mass 2", 1., 0.1, 10., 0.01);
  GetParam(kParamMass3)->InitDouble("Mass 3", 1., 0.1, 10., 0.01);
  GetParam(kParamStartRadius)->InitDouble("Start Radius", 1.5, 0.3, 3., 0.01);
  GetParam(kParamBoxSize)->InitDouble("Box Size", 4., 2., 10., 0.01);
  GetParam(kParamCaptureWindow)->InitDouble("Capture Window", 0.3, 0.02, 2., 0.001, "s");
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
    IRECT area = bounds.GetPadded(-20.f);

    constexpr int kRows = 2, kCols = 4;
    auto Cell = [&](int row, int col) { return area.GetGridCell(row, col, kRows, kCols); };

    pGraphics->AttachControl(new IVKnobControl(Cell(0, 0).GetCentredInside(55.f), kParamMass1, "Mass 1"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 1).GetCentredInside(55.f), kParamMass2, "Mass 2"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 2).GetCentredInside(55.f), kParamMass3, "Mass 3"));
    pGraphics->AttachControl(new IVKnobControl(Cell(0, 3).GetCentredInside(55.f), kParamStartRadius, "Radius"));

    pGraphics->AttachControl(new IVKnobControl(Cell(1, 0).GetCentredInside(55.f), kParamBoxSize, "Box Size"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 1).GetCentredInside(55.f), kParamCaptureWindow, "Window"));
    pGraphics->AttachControl(new IVMenuButtonControl(Cell(1, 2).GetCentredInside(90.f, 30.f), kParamTableSize, "Table Size"));
    pGraphics->AttachControl(new IVKnobControl(Cell(1, 3).GetCentredInside(55.f), kParamBitDepth, "Bit Depth"));

    // Bouton Capture, centre sous la grille
    IRECT captureArea = bounds.GetFromBottom(60.f);
    pGraphics->AttachControl(new IVButtonControl(captureArea.GetCentredInside(140.f, 40.f),
      [this](IControl* pCaller) { RequestCapture(); }, "Capture"));
  };
#endif

#if IPLUG_DSP
  OnReset();
#endif
}

#if IPLUG_DSP

void TroisCorpsWave::DoCapture()
{
  double m1 = GetParam(kParamMass1)->Value();
  double m2 = GetParam(kParamMass2)->Value();
  double m3 = GetParam(kParamMass3)->Value();
  double startRadius = GetParam(kParamStartRadius)->Value();
  double boxSize = GetParam(kParamBoxSize)->Value();
  double captureWindow = GetParam(kParamCaptureWindow)->Value();

  mEngine.SetMasses(m1, m2, m3);
  mEngine.SetBoxSize(boxSize);
  mEngine.ResetToTriangle(startRadius);

  int nCaptured = mEngine.CaptureBody1X(captureWindow, mRawCaptureBuffer, kMaxRawCapture);

  int tableSizeIdx = (int)GetParam(kParamTableSize)->Value();
  int tableSize = 64 << tableSizeIdx; // 0->64, 1->128, ... 6->4096

  if (nCaptured > 1)
    mOsc.SetTable(mRawCaptureBuffer, nCaptured, tableSize);
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
