
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vTroisCorpsWave
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vTroisCorpsWave
#import <TroisCorpsWaveAU/IPlugAUViewController.h>
#import <TroisCorpsWaveAU/IPlugAUAudioUnit.h>

//! Project version number for TroisCorpsWaveAU.
FOUNDATION_EXPORT double TroisCorpsWaveAUVersionNumber;

//! Project version string for TroisCorpsWaveAU.
FOUNDATION_EXPORT const unsigned char TroisCorpsWaveAUVersionString[];

@class IPlugAUViewController_vTroisCorpsWave;
