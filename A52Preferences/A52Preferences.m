#import "A52Preferences.h"

#include "a52.h"

//Old
#define USE_STEREO_KEY @"useStereoOverDolby"
#define USE_DPLII_KEY @"useDolbyProLogicII"

#define DYNAMIC_RANGE_KEY @"dynamicRange"
#define PASSTHROUGH_KEY @"attemptPassthrough"
#define TWO_CHANNEL_KEY @"twoChannelMode"

@interface A52Preferences (private)
- (void)setAC3DynamicRange:(float)newVal;
@end

@implementation A52Preferences

- (id)init
{
	self = [super init];
	if(self == nil)
		return nil;
	
	defaults = [[NSUserDefaults standardUserDefaults] retain];
	return self;
}

- (void)upgradeOldPrefs
{
	if([defaults boolForKey:USE_STEREO_KEY])
		twoChannelMode = A52_STEREO;
	else if([defaults boolForKey:USE_DPLII_KEY])
		twoChannelMode = A52_DOLBY | A52_USE_DPLII;
	else
		twoChannelMode = A52_DOLBY;
	
	[defaults setInteger:twoChannelMode forKey:TWO_CHANNEL_KEY];
}

- (void)awakeFromNib
{
	if([defaults objectForKey:DYNAMIC_RANGE_KEY] != nil)
		[self setAC3DynamicRange:[defaults floatForKey:DYNAMIC_RANGE_KEY]];
	else
		[self setAC3DynamicRange:1.0];
	if([defaults objectForKey:TWO_CHANNEL_KEY] != nil)
	{
		twoChannelMode = [defaults integerForKey:TWO_CHANNEL_KEY];

		/* sanity checks */
		if(twoChannelMode & A52_CHANNEL_MASK & 0xf7 != 2)
		{
			/* matches 2 and 10, which is Stereo and Dolby */
			twoChannelMode = A52_DOLBY;
		}
		twoChannelMode &= ~A52_ADJUST_LEVEL & ~A52_LFE;
	}
	else
	{
		[self upgradeOldPrefs];
	}
	[defaults removeObjectForKey:USE_STEREO_KEY];
	[defaults removeObjectForKey:USE_DPLII_KEY];
	switch(twoChannelMode)
	{
		case A52_STEREO:
			[popup_OutputMode selectItemAtIndex:0];
			break;
		case A52_DOLBY:
			[popup_OutputMode selectItemAtIndex:1];
			break;
		case A52_DOLBY | A52_USE_DPLII:
			[popup_OutputMode selectItemAtIndex:2];
			break;
		default:
			[popup_OutputMode selectItemAtIndex:3];
			break;
	}
}

- (void)dealloc
{
	[defaults release];
	[super dealloc];
}

- (IBAction)setAC3DynamicRangePopup:(id)sender
{
	int selected = [popup_ac3DynamicRangeType indexOfSelectedItem];
	switch(selected)
	{
		case 0:
			[self setAC3DynamicRange:1.0];
			break;
		case 1:
			[self setAC3DynamicRange:2.0];
			break;
		case 3:
			savedDynValue = dynValue;
			[NSApp beginSheet:window_dynRangeSheet modalForWindow:window_mainWindow modalDelegate:nil didEndSelector:nil contextInfo:NULL];
			break;
		default:
			break;
	}
}

- (IBAction)set2ChannelModePopup:(id)sender;
{
	int selected = [popup_OutputMode indexOfSelectedItem];
	switch(selected)
	{
		case 0:
			twoChannelMode = A52_STEREO;
			break;
		case 1:
			twoChannelMode = A52_DOLBY;
			break;
		case 2:
			twoChannelMode = A52_DOLBY | A52_USE_DPLII;
			break;
		case 3:
			twoChannelMode = 0;
		default:
			break;
	}	
}

- (void)setAC3DynamicRange:(float)newVal
{
    if(newVal > 4.0)
        newVal = 4.0;
    if(newVal < 0.0)
        newVal = 0.0;
    
    dynValue = newVal;
    [textField_ac3DynamicRangeValue setFloatValue:newVal];
    [slider_ac3DynamicRangeSlider setFloatValue:newVal];
	if(newVal == 1.0)
		[popup_ac3DynamicRangeType selectItemAtIndex:0];
	else if(newVal == 2.0)
		[popup_ac3DynamicRangeType selectItemAtIndex:1];
	else
		[popup_ac3DynamicRangeType selectItemAtIndex:3];
}

- (IBAction)setAC3DynamicRangeValue:(id)sender
{
    float newVal = [textField_ac3DynamicRangeValue floatValue];
    
    [self setAC3DynamicRange:newVal];
}

- (IBAction)setAC3DynamicRangeSlider:(id)sender
{
    float newVal = [slider_ac3DynamicRangeSlider floatValue];
    
    [self setAC3DynamicRange:newVal];
}

- (IBAction)cancelDynRangeSheet:(id)sender
{
	[self setAC3DynamicRange:savedDynValue];
	[NSApp endSheet:window_dynRangeSheet];
	[window_dynRangeSheet orderOut:self];
}

- (IBAction)saveDynRangeSheet:(id)sender;
{
	[NSApp endSheet:window_dynRangeSheet];
	[window_dynRangeSheet orderOut:self];
}

- (IBAction)cancel:(id)sender
{
	[[NSApplication sharedApplication] terminate:nil];
}

- (IBAction)save:(id)sender
{
	[defaults setFloat:dynValue forKey:DYNAMIC_RANGE_KEY];
	[defaults setInteger:twoChannelMode forKey:TWO_CHANNEL_KEY];
	[defaults synchronize];
	[[NSApplication sharedApplication] terminate:nil];
}

@end
