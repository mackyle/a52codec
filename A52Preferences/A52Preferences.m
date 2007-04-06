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
		[button_stereo setIntValue:1];
	else
		[button_stereo setIntValue:0];
}

- (void)dealloc
{
	[defaults release];
	[super dealloc];
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

- (IBAction)cancel:(id)sender
{
	[[NSApplication sharedApplication] terminate:nil];
}

- (IBAction)save:(id)sender
{
	[defaults setFloat:dynValue forKey:@"dynamicRange"];
	if([button_stereo intValue] != 0)
		[defaults setBool:YES forKey:@"useStereoOverDolby"];
	else
		[defaults setBool:NO forKey:@"useStereoOverDolby"];
	[defaults synchronize];
	[[NSApplication sharedApplication] terminate:nil];
}

@end
