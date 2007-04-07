#import "A52Preferences.h"

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

- (void)awakeFromNib
{
	if([defaults objectForKey:@"dynamicRange"] != nil)
		[self setAC3DynamicRange:[defaults floatForKey:@"dynamicRange"]];
	else
		[self setAC3DynamicRange:1.0];
	if([defaults boolForKey:@"useStereoOverDolby"])
	{
		useStereo = YES;
		useDPL2 = NO;
		[popup_2ChannelMode selectItemAtIndex:0];
	}
	else if([defaults boolForKey:@"useDolbyProLogicII"])
	{
		useStereo = NO;
		useDPL2 = YES;
		[popup_2ChannelMode selectItemAtIndex:2];
	}
	else
	{
		useStereo = NO;
		useDPL2 = NO;
		[popup_2ChannelMode selectItemAtIndex:1];		
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
		case 2:
			savedDynValue = dynValue;
			[NSApp beginSheet:window_dynRangeSheet modalForWindow:window_mainWindow modalDelegate:nil didEndSelector:nil contextInfo:NULL];
			break;
		default:
			break;
	}
}

- (IBAction)set2ChannelModePopup:(id)sender;
{
	int selected = [popup_2ChannelMode indexOfSelectedItem];
	switch(selected)
	{
		case 0:
			useStereo = YES;
			useDPL2 = NO;
			break;
		case 1:
			useStereo = NO;
			useDPL2 = NO;
			break;
		case 2:
			useStereo = NO;
			useDPL2 = YES;
			break;
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
		[popup_ac3DynamicRangeType selectItemAtIndex:2];
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
	[defaults setFloat:dynValue forKey:@"dynamicRange"];
	[defaults setBool:useStereo forKey:@"useStereoOverDolby"];
	[defaults setBool:useDPL2 forKey:@"useDolbyProLogicII"];
	[defaults synchronize];
	[[NSApplication sharedApplication] terminate:nil];
}

@end
