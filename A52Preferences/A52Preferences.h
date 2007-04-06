/* A52PreferencesWindowController */

#import <Cocoa/Cocoa.h>

@interface A52Preferences : NSObject
{
    IBOutlet NSTextField                *textField_ac3DynamicRangeValue;
    IBOutlet NSSlider                   *slider_ac3DynamicRangeSlider;
    IBOutlet NSButton					*button_stereo;
	
	NSUserDefaults						*defaults;
	float								dynValue;
}

- (IBAction)setAC3DynamicRangeValue:(id)sender;
- (IBAction)setAC3DynamicRangeSlider:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)save:(id)sender;
@end
