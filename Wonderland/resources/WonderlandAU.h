
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vWonderland
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vWonderland
#import <WonderlandAU/IPlugAUViewController.h>
#import <WonderlandAU/IPlugAUAudioUnit.h>

//! Project version number for WonderlandAU.
FOUNDATION_EXPORT double WonderlandAUVersionNumber;

//! Project version string for WonderlandAU.
FOUNDATION_EXPORT const unsigned char WonderlandAUVersionString[];

@class IPlugAUViewController_vWonderland;
