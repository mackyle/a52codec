/* A52PreferencesWindowController */

#import <Cocoa/Cocoa.h>

@interface A52Preferences : NSObject
{
    IBOutlet NSButton *NSButton_dynamicRange;
    IBOutlet NSButton *NSButton_stereo;
	
	NSUserDefaults	*defaults;
}
- (IBAction)cancel:(id)sender;
- (IBAction)save:(id)sender;
@end
