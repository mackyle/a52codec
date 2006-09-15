#import "A52Preferences.h"

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
	if([defaults boolForKey:@"dynamicRange"])
		[NSButton_dynamicRange setIntValue:1];
	else
		[NSButton_dynamicRange setIntValue:0];
	if([defaults boolForKey:@"useStereoOverDolby"])
		[NSButton_stereo setIntValue:1];
	else
		[NSButton_stereo setIntValue:0];
}

- (void)dealloc
{
	[defaults release];
	[super dealloc];
}

- (IBAction)cancel:(id)sender
{
	[[NSApplication sharedApplication] terminate:nil];
}

- (IBAction)save:(id)sender
{
	if([NSButton_dynamicRange intValue] != 0)
		[defaults setBool:YES forKey:@"dynamicRange"];
	else
		[defaults setBool:NO forKey:@"dynamicRange"];
	if([NSButton_stereo intValue] != 0)
		[defaults setBool:YES forKey:@"useStereoOverDolby"];
	else
		[defaults setBool:NO forKey:@"useStereoOverDolby"];
	[defaults synchronize];
	[[NSApplication sharedApplication] terminate:nil];
}

@end
