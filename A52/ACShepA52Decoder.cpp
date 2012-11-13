/*
	A52 / AC-3 Decompression Codec
	2004, Shepmaster <shepmaster@applesolutions.com>
	
	Uses code from the liba52 project and the CoreAudio SDK.
*/

//=============================================================================
//	Includes
//=============================================================================

#include "ACShepA52Decoder.h"
#include "ACCodecDispatch.h"
#include "CAStreamBasicDescription.h"
#include "CASampleTools.h"
#include "CADebugMacros.h"

//=============================================================================
//	ACShepA52Decoder
//=============================================================================

#define kAudioFormatAVIAC3	0x6D732000
#define MY_APP_DOMAIN CFSTR("com.cod3r.a52codec")

//Old
#define USE_STEREO_KEY CFSTR("useStereoOverDolby")
#define USE_DPLII_KEY CFSTR("useDolbyProLogicII")

#define DYNAMIC_RANGE_KEY CFSTR("dynamicRange")
#define PASSTHROUGH_KEY CFSTR("attemptPassthrough")
#define TWO_CHANNEL_KEY CFSTR("twoChannelMode")

void ACShepA52Decoder::UpgradeOldPrefs()
{
	CFStringRef myApp = MY_APP_DOMAIN;
	int useStereoOverDolby = 0;
	CFTypeRef stereo = CFPreferencesCopyAppValue(USE_STEREO_KEY, myApp);
	if(stereo != NULL)
	{
		CFTypeID type = CFGetTypeID(stereo);
		if(type == CFStringGetTypeID())
			useStereoOverDolby = CFStringGetIntValue((CFStringRef)stereo);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)stereo, kCFNumberIntType, &useStereoOverDolby);
		else if(type == CFBooleanGetTypeID())
			useStereoOverDolby = CFBooleanGetValue((CFBooleanRef)stereo);
		CFRelease(stereo);
	}
	
	int useDPL2 = 0;
	CFTypeRef dpl2 = CFPreferencesCopyAppValue(USE_DPLII_KEY, myApp);
	if(dpl2 != NULL)
	{
		CFTypeID type = CFGetTypeID(dpl2);
		if(type == CFStringGetTypeID())
			useDPL2 = CFStringGetIntValue((CFStringRef)dpl2);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)dpl2, kCFNumberIntType, &useDPL2);
		else if(type == CFBooleanGetTypeID())
			useDPL2 = CFBooleanGetValue((CFBooleanRef)dpl2);
		CFRelease(dpl2);
	}
	
	if(useStereoOverDolby)
		TwoChannelMode = A52_STEREO;
	else if(useDPL2)
		TwoChannelMode = A52_DOLBY | A52_USE_DPLII;
	else
		TwoChannelMode = A52_DOLBY;
	
	CFNumberRef resultingMode = CFNumberCreate(NULL, kCFNumberIntType, &TwoChannelMode);
	CFPreferencesSetAppValue(TWO_CHANNEL_KEY, resultingMode, myApp);
	CFRelease(resultingMode);
	CFPreferencesAppSynchronize(myApp);
}

ACShepA52Decoder::ACShepA52Decoder(OSType theSubType) : ACShepA52Codec(76800, theSubType) {
	
	//еее	One issue to talk about here is how do we represent the fact that this
	//еее	decoder doesn't care about the number of channels or the sample rate?
	//еее	For now, I've implemented it by specifying 0 for the values that don't
	//еее	matter. The issue is that 0 can also mean that the value is unknown or
	//еее	doesn't apply.
	//еее
	//еее	Below are examples of both usages, in both the available input and output
	//еее	formats. Since the number of channels is 0 (because this decoder doesn't
	//еее	care), a lot of the dependant values, like bytes per frame, are unknowable.
	//еее
	//еее	The reason why this is an issue is that code that tries to figure out how
	//еее	to hook this decoder up to other converters is going to have to be very
	//еее	careful about being strict enough with format matching that it can always
	//еее	be correct, but not too strict that it treats the situations thus created
	//еее	as bad matches or, worse yet, an error.
	
	
	/*
	 CAStreamBasicDescription(		double inSampleRate,		UInt32 inFormatID,
									UInt32 inBytesPerPacket,	UInt32 inFramesPerPacket,
									UInt32 inBytesPerFrame,		UInt32 inChannelsPerFrame,
									UInt32 inBitsPerChannel,	UInt32 inFormatFlags);
	 */
	
#ifdef __BIG_ENDIAN__
	kIntPCMOutFormatFlag   = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsPacked;
	kFloatPCMOutFormatFlag = kLinearPCMFormatFlagIsFloat		 | kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsPacked;
#else
	kIntPCMOutFormatFlag   = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
	kFloatPCMOutFormatFlag = kLinearPCMFormatFlagIsFloat		 | kLinearPCMFormatFlagIsPacked;
#endif
	CFStringRef myApp = MY_APP_DOMAIN;
	
	CFPreferencesAppSynchronize(myApp);
	CFTypeRef dynRange = CFPreferencesCopyAppValue(DYNAMIC_RANGE_KEY, myApp);
	if(dynRange != NULL)
	{
		CFTypeID type = CFGetTypeID(dynRange);
		if(type == CFStringGetTypeID())
			dynamicRangeCompression = CFStringGetDoubleValue((CFStringRef)dynRange);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)dynRange, kCFNumberDoubleType, &dynamicRangeCompression);
        else
			dynamicRangeCompression = 1;
		CFRelease(dynRange);
	}
	else
		dynamicRangeCompression = 1;  //no compression
	
	CFTypeRef pass = CFPreferencesCopyAppValue(PASSTHROUGH_KEY, myApp);
	if(pass != NULL)
	{
		CFTypeID type = CFGetTypeID(pass);
		if(type == CFStringGetTypeID())
			passthrough = CFStringGetIntValue((CFStringRef)pass);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)pass, kCFNumberIntType, &passthrough);
		else
			passthrough = 0;
		CFRelease(pass);
	}
	else
		passthrough = 0;
	
	CFTypeRef twochan = CFPreferencesCopyAppValue(TWO_CHANNEL_KEY, myApp);
	if(twochan != NULL)
	{
		CFTypeID type = CFGetTypeID(twochan);
		if(type == CFStringGetTypeID())
			TwoChannelMode = CFStringGetIntValue((CFStringRef)twochan);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)twochan, kCFNumberIntType, &TwoChannelMode);
		else
			TwoChannelMode = 0;
		/* sanity checks */
		if(TwoChannelMode & A52_CHANNEL_MASK & 0xf7 != 2)
		{
			/* matches 2 and 10, which is Stereo and Dolby */
			TwoChannelMode = A52_DOLBY;
		}
		TwoChannelMode &= ~A52_ADJUST_LEVEL & ~A52_LFE;
		CFRelease(twochan);
	}
	else
		UpgradeOldPrefs();
	CFPreferencesSetAppValue(USE_STEREO_KEY, NULL, myApp);
	CFPreferencesSetAppValue(USE_DPLII_KEY, NULL, myApp);

	static int sample_rates[] = {48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 6000, 5512, 4000};
	for (int sample_index = 0; sample_index < 12; sample_index ++)
	{
		for (int channels = 1; channels <= 6; channels++) {
			//	This decoder only takes an A/52 or AC-3 stream as it's input
			CAStreamBasicDescription theInputFormat1(sample_rates[sample_index], kAudioFormatAC3, 0, 256*6, 0, channels, 0, 0);
			AddInputFormat(theInputFormat1);
			CAStreamBasicDescription theInputFormat2(sample_rates[sample_index], kAudioFormatAVIAC3, 0, 256*6, 0, channels, 0, 0);
			AddInputFormat(theInputFormat2);
			CAStreamBasicDescription theInputFormat3(sample_rates[sample_index], 'AC-3', 0, 256*6, 0, channels, 0, 0);
			AddInputFormat(theInputFormat3);
			
			// if two channel mode is engaged and we are not passing through, only set output for 2 channels
			if(!passthrough && TwoChannelMode != 0 && channels != 2)
				continue;
			// Output 16-Bit Ints
			CAStreamBasicDescription theOutputFormat1(sample_rates[sample_index], kAudioFormatLinearPCM, 0, 1, 0, channels, 16, kIntPCMOutFormatFlag);
			AddOutputFormat(theOutputFormat1);
			
			if(!passthrough)
			{
				// And 32-Bit
				CAStreamBasicDescription theOutputFormat2(sample_rates[sample_index], kAudioFormatLinearPCM, 0, 1, 0, channels, 32, kIntPCMOutFormatFlag);
				AddOutputFormat(theOutputFormat2);
				
				// And floats
				CAStreamBasicDescription theOutputFormat3(sample_rates[sample_index], kAudioFormatLinearPCM, 0, 1, 0, channels, 32, kFloatPCMOutFormatFlag);
				AddOutputFormat(theOutputFormat3);
			}
		}
	}
	
	total_bytes = 0;
	total_frames = 0;
	decoder_state = NULL;
	firstInput = true;
	
	remainingBytesFromLastFrame = 0;
	beginningOfIncompleteHeaderSize = 0;
		
	layoutTag = kAudioChannelLayoutTag_MPEG_5_1_A;
	int initialMap[6] = {3, 0, 2, 1, 4, 5};
	memcpy(fullChannelMap, initialMap, sizeof(fullChannelMap));
	//fprintf(stderr, "Instance %p alloced\n", this);

	//fprintf(stderr, "ACShepA52Decoder::Constructor: Number of input formats supported: %lu\n", GetNumberSupportedInputFormats());
	//fprintf(stderr, "ACShepA52Decoder::Constructor: Number of output formats supported: %lu\n", GetNumberSupportedOutputFormats());
}

ACShepA52Decoder::~ACShepA52Decoder() {
	// fprintf(stderr, "ACShepA52Decoder::Destructor: Total Frames: %ld\n", total_frames);
	// fprintf(stderr, "ACShepA52Decoder::Destructor: Total Output Bytes: %ld\n", total_bytes);
	// fprintf(stderr, "Instance %p freed\n", this);
}

UInt32 ACShepA52Decoder::ParseCookieAtom(const uint8_t* inAtom, UInt32 inAtomMaxSize)
{
	if(inAtomMaxSize < 8)
		//Invalid; atom must be at least 8 bytes.
		return inAtomMaxSize;
	const UInt32 *atomElements = reinterpret_cast<const UInt32 *>(inAtom);
	UInt32 atomSize = EndianU32_BtoN(atomElements[0]);
	UInt32 atomType = EndianU32_BtoN(atomElements[1]);
	
	if(atomSize > inAtomMaxSize)
		return inAtomMaxSize;
	
	switch (atomType) {
		case 'dac3': {
			if(atomSize < 11)
				//dac3 is at least 3 bytes in size
				break;
			//Cookie only found in .mp4, so set layout
			//A52's layout is LFE, left, center, right, left surround, right surround.
			//Apple's .mp4 sets a layout of L C R Ls Rs LFE
			int acmod = (inAtom[9] & 0x3f) >> 3;
			int layout[6] = {5, 0, 1, 2, 3, 4};
			switch (acmod) {
				case 0:	//2 channels
				case 2: //L R
				case 3: //L R S
				case 6: //L R Ls Rs
					//No Center
					layout[3] = 1;
					break;
				case 1:
					//Center only
					layout[2] = 0;
					break;
				default:
					//All others correct
					break;
			}
			memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
		}
			break;
		default:
			break;
	}
	
	return atomSize;
}

void ACShepA52Decoder::ParseCookie(const uint8_t* inMagicCookie, UInt32 inMagicCookieByteSize)
{
	UInt32 offset = 0;
	while(offset < inMagicCookieByteSize) {
		offset += ParseCookieAtom(inMagicCookie + offset, inMagicCookieByteSize - offset);
	}
}

void ACShepA52Decoder::Initialize(const AudioStreamBasicDescription* inInputFormat,
									 const AudioStreamBasicDescription* inOutputFormat,
									 const void* inMagicCookie, UInt32 inMagicCookieByteSize) {
	
	ACShepA52Codec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
	
	//fprintf(stderr, "ACShepA52Decoder::Initialize: Initializing, magic cookie size is %lu\n", inMagicCookieByteSize);
	if(inMagicCookie)
		ParseCookie(static_cast<const uint8_t *> (inMagicCookie), inMagicCookieByteSize);
	
	// Library setup
	a52_accel(A52_ACCEL_DJBFFT);
	decoder_state = a52_init(); // No optimizations here
	if (decoder_state == NULL) {
		fprintf(stderr, "ACShepA52Decoder::Initialize: Umm... liba52 could not be loaded!\n");
		fprintf(stderr, "ACShepA52Decoder::Initialize: I don't know what happens next...\n");
		//TODO: return some error
	}
	firstInput = true;
}

void ACShepA52Decoder::Uninitialize() {
	if (decoder_state != NULL) {
		a52_free(decoder_state);
		decoder_state = NULL;
	}
	
	//fprintf(stderr, "ACShepA52Decoder::Uninitialize: Uninitializing\n");
	
	//	let our base class clean up it's internal state
	ACShepA52Codec::Uninitialize();
}


void ACShepA52Decoder::Reset() {
	//fprintf(stderr, "ACShepA52Decoder::Reset: Resetting\n");
	
	if (decoder_state != NULL) {
		a52_free(decoder_state);
	}
	a52_accel(A52_ACCEL_DJBFFT);
	decoder_state = a52_init();
	
	firstInput = true;
	remainingBytesFromLastFrame = beginningOfIncompleteHeaderSize = 0;
	
	//	let our base class clean up it's internal state
	ACShepA52Codec::Reset();
}

void ACShepA52Decoder::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, Boolean& outWritable) {
	//fprintf(stderr, "ACShepA52Decoder::GetPropertyInfo: Asking for property %.*s\n", (int) sizeof(inPropertyID), (char*)&inPropertyID);
	
	switch(inPropertyID)
	{
		case kAudioCodecPropertyOutputChannelLayout:
			outPropertyDataSize = sizeof(AudioChannelLayout);
			outWritable = false;
			break;
		case kAudioCodecPropertyInputChannelLayout:
			outPropertyDataSize = sizeof(AudioChannelLayout);
			outWritable = true;
			break;
		default:
		ACShepA52Codec::GetPropertyInfo(inPropertyID, outPropertyDataSize, outWritable);
	}
}

void ACShepA52Decoder::GetProperty(AudioCodecPropertyID inPropertyID, UInt32& ioPropertyDataSize, void* outPropertyData) {
	
	//fprintf(stderr, "ACShepA52Decoder::GetProperty: Asking for property %.*s\n", (int) sizeof(inPropertyID), (char*)&inPropertyID);
	
	switch(inPropertyID)
	{
#if TARGET_OS_MAC
		case kAudioCodecPropertyNameCFString:
		{
			if (ioPropertyDataSize != sizeof(CFStringRef)) {
				CODEC_THROW(kAudioCodecBadPropertySizeError);
			}
			
			CFStringRef name = CFCopyLocalizedStringFromTableInBundle(CFSTR("Shepmaster's A52 decoder"), CFSTR("CodecNames"), GetCodecBundle(), CFSTR(""));
			*(CFStringRef*)outPropertyData = name;
			break; 
		}
#endif
		case kAudioCodecPropertyOutputChannelLayout:
			if(ioPropertyDataSize != sizeof(AudioChannelLayout))  CODEC_THROW(kAudioCodecBadPropertySizeError);
			memset(outPropertyData, 0, sizeof(AudioChannelLayout));
			(reinterpret_cast<AudioChannelLayout*>(outPropertyData))->mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_5_1_A;
			break;
		case kAudioCodecPropertyInputChannelLayout:
			if(ioPropertyDataSize != sizeof(AudioChannelLayout))  CODEC_THROW(kAudioCodecBadPropertySizeError);
			memset(outPropertyData, 0, sizeof(AudioChannelLayout));
			(reinterpret_cast<AudioChannelLayout*>(outPropertyData))->mChannelLayoutTag = layoutTag;
			break;
			
		default:
			
			ACShepA52Codec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}



void ACShepA52Decoder::SetProperty(AudioCodecPropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	//fprintf(stderr, "ACShepA52Decoder::SetProperty: Asking for property %.*s\n", (int) sizeof(inPropertyID), (char*)&inPropertyID);

	switch(inPropertyID) {
		case kAudioCodecPropertyInputChannelLayout:
		//This doesn't work because Apple's use of Core Audio is broken: http://openradar.appspot.com/6921431
		//It is here in case Apple fixes it, then we don't have to do another release.
		{
			AudioChannelLayoutTag tag = (reinterpret_cast<const AudioChannelLayout*>(inPropertyData))->mChannelLayoutTag;
			//fprintf(stderr, "Instance %p told that input tag is %lx\n", this, tag);
			switch(tag)
			{
				/* These are all the up to 5.1 layout tags defined in core audio
				 * In layouts, missing channels are skiped.
				 * Center surround is listed as left surround since a52 has at most 2 rear channels*/
				//A52's layout is LFE, left, center, right, left surround, right surround.
				case kAudioChannelLayoutTag_MPEG_1_0:
				case kAudioChannelLayoutTag_MPEG_3_0_B:
				case kAudioChannelLayoutTag_MPEG_4_0_B:
				case kAudioChannelLayoutTag_MPEG_5_0_D:
				case kAudioChannelLayoutTag_MPEG_5_1_D:
				//center, left, right, left surround, right surround, LFE
				{
					int layout[6] = {5, 1, 0, 2, 3, 4};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_MPEG_2_0:
				case kAudioChannelLayoutTag_MPEG_3_0_A:
				case kAudioChannelLayoutTag_MPEG_4_0_A:
				case kAudioChannelLayoutTag_MPEG_5_0_A:
				//left, right, center, left surround, right surround, (LFE)
				{
					int layout[6] = {5, 0, 2, 1, 3, 4};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_MPEG_5_0_B:
				case kAudioChannelLayoutTag_MPEG_5_1_B:
				case kAudioChannelLayoutTag_DVD_18:
				//left, right, left surround, right surround, center, LFE
				{
					int layout[6] = {5, 0, 4, 1, 2, 3};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_MPEG_5_0_C:
				case kAudioChannelLayoutTag_MPEG_5_1_C:
				//left, center, right, left surround, right surround, LFE
				{
					int layout[6] = {5, 0, 1, 2, 3, 4};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_MPEG_5_1_A:
				case kAudioChannelLayoutTag_DVD_10:
				case kAudioChannelLayoutTag_DVD_11:
				//left, right, center, LFE, left surround, right surround
				{
					int layout[6] = {3, 0, 2, 1, 4, 5};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_ITU_2_1:
				case kAudioChannelLayoutTag_ITU_2_2:
				//left, right, left surround, right surround, (center, LFE)
				{
					int layout[6] = {5, 0, 4, 1, 2, 3};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
				case kAudioChannelLayoutTag_DVD_4:
				case kAudioChannelLayoutTag_DVD_5:
				case kAudioChannelLayoutTag_DVD_6:
				//left, right, LFE, left surround, right surround, (center)
				{
					int layout[6] = {2, 0, 5, 1, 3, 4};
					memcpy(fullChannelMap, layout, sizeof(fullChannelMap));
					break;
				}
			}
			layoutTag = tag;
			break;
		}
		default:
		
		ACShepA52Codec::SetProperty(inPropertyID, inPropertyDataSize, inPropertyData);
	}
}

void ACShepA52Decoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat) {
	/*
	fprintf(stderr, "--------------------\n");
	fprintf(stderr, "Setting input format\n");
	fprintf(stderr, "Rate: %f\n", inInputFormat.mSampleRate);
	fprintf(stderr, "Format ID: %s\n", inInputFormat.mFormatID == kAudioFormatAC3 ? "kAudioFormatAC3" : "not kAudioFormatAC3");
	fprintf(stderr, "Bytes per packet: %ld\n", inInputFormat.mBytesPerPacket);
	fprintf(stderr, "Frames per packet: %ld\n", inInputFormat.mFramesPerPacket);
	fprintf(stderr, "Bytes per frame: %ld\n", inInputFormat.mBytesPerFrame);
	fprintf(stderr, "Channels: %ld\n", inInputFormat.mChannelsPerFrame);
	fprintf(stderr, "Bits per Channel: %ld\n", inInputFormat.mBitsPerChannel);
	fprintf(stderr, "--------------------\n");
	*/
	
	
	if(mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	//	check to make sure the input format is legal
	if(inInputFormat.mFormatID != kAudioFormatAC3 && inInputFormat.mFormatID != kAudioFormatAVIAC3 && inInputFormat.mFormatID != 'AC-3') {
		DebugMessage("ACShepA52Decoder::SetFormats: only support A/52 and AC-3 for input");
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	//fprintf(stderr, "ACShepA52Decoder::SetFormats: input set to %lx, channels: %ld\n", inInputFormat.mFormatID, inInputFormat.mChannelsPerFrame);
	
	//	tell our base class about the new format
	ACShepA52Codec::SetCurrentInputFormat(inInputFormat);
	
	//fprintf(stderr, "ACShepA52Decoder::SetCurrentInputFormat: Exiting function\n");
}



void ACShepA52Decoder::SetCurrentOutputFormat(const AudioStreamBasicDescription& inOutputFormat) {
	
	/*
	fprintf(stderr, "--------------------\n");
	fprintf(stderr, "Setting output format\n");
	fprintf(stderr, "Rate: %f\n", inOutputFormat.mSampleRate);
	fprintf(stderr, "Format ID: %s\n", inOutputFormat.mFormatID == kAudioFormatLinearPCM ? "kAudioFormatLinearPCM" : "not kAudioFormatLinearPCM");
	fprintf(stderr, "Bytes per packet: %ld\n", inOutputFormat.mBytesPerPacket);
	fprintf(stderr, "Frames per packet: %ld\n", inOutputFormat.mFramesPerPacket);
	fprintf(stderr, "Bytes per frame: %ld\n", inOutputFormat.mBytesPerFrame);
	fprintf(stderr, "Channels: %ld\n", inOutputFormat.mChannelsPerFrame);
	fprintf(stderr, "Bits per Channel: %ld\n", inOutputFormat.mBitsPerChannel);
	fprintf(stderr, "--------------------\n");
	*/
	
	
	//fprintf(stderr, "ACShepA52Decoder::SetFormats: output set to %lx, channels: %ld\n", inOutputFormat.mFormatID, inOutputFormat.mChannelsPerFrame);

	if(mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	if(inOutputFormat.mFormatID != kAudioFormatLinearPCM) {
		DebugMessage("ACShepA52Decoder::SetFormats: only supports linear PCM for output");
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	if (! (inOutputFormat.mFormatFlags == kIntPCMOutFormatFlag || inOutputFormat.mFormatFlags == kFloatPCMOutFormatFlag)) {
		DebugMessage("ACShepA52Decoder::SetFormats: does not support this form of PCM output");
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
	if (inOutputFormat.mFormatFlags == kIntPCMOutFormatFlag) {
		if (! (inOutputFormat.mBitsPerChannel == 16 || inOutputFormat.mBitsPerChannel == 32)) {
			DebugMessage("ACShepA52Decoder::SetFormats: only supports 16 and 32 bit integer PCM");
			CODEC_THROW(kAudioCodecUnsupportedFormatError);
		}
	}
		
	//	tell our base class about the new format
	ACShepA52Codec::SetCurrentOutputFormat(inOutputFormat);
	//fprintf(stderr, "ACShepA52Decoder::SetCurrentOutputFormat: Exiting function\n");
}

static unsigned int ChannelCount(int a52_flags)
{
	int total_channels = 0;
	
	switch (a52_flags & A52_CHANNEL_MASK) {
		
		case A52_CHANNEL:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found two independant monaural audio\n");
			total_channels = 2;
			break;
			
		case A52_CHANNEL1:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found first of two independant monaural channel audio\n");
			total_channels = 1;
			break;
			
		case A52_CHANNEL2:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found second of two independant monaural channel audio\n");
			total_channels = 1;
			break;
			
		case A52_MONO:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found monaural audio\n");
			total_channels = 1;
			break;
			
		case A52_STEREO:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found stereo audio\n");
			total_channels = 2;
			break;
			
		case A52_DOLBY:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found Dolby Surround sound audio\n");
			total_channels = 2;
			break;
			
		case A52_3F:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front channel audio\n");
			total_channels = 3;
			break;
			
		case A52_2F1R:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 2 front & 1 back channel audio\n");
			total_channels = 3;
			break;
			
		case A52_3F1R:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front & 1 back channel audio\n");
			total_channels = 4;
			break;
			
		case A52_2F2R:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 2 front & 2 back channel audio\n");
			total_channels = 4;
			break;
			
		case A52_3F2R:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front & 2 back channel audio\n");
			total_channels = 5;
			break;

		default:
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: No audio stream information found, probably not good!\n");
			break;
	}
	
	if (a52_flags & A52_LFE) {
		//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found LFE channel in audio\n");
		total_channels ++;
	}
	
	return total_channels;
}

/*
 * Custom Dynamic range compression
 * My logic here:
 * Two cases
 * 1) The user requested a compression of 1 or less
 *		return the typical power rule
 * 2) The user requested a compression of more than 1 (decompression)
 *		If the stream's requested compression is less than 1.0 (loud sound), return the normal compression
 *		If the stream's requested compression is more than 1.0 (soft sound), use power rule (which will make it louder in this case).
 */
static sample_t dynrng_call (sample_t c, void *data)
{
	double *level = (double *)data;
	float levelToUse = (float)*level;
	if(c > 1.0 || levelToUse <= 1.0)
		return powf(c, levelToUse);
	else
		return c;
}

// Output order of liba52: LFE, left, center, right, left surround, right surround.
// Channels missing are skipped

void ACShepA52Decoder::getChannelMap(int a52_flags, int chanMap[6])
{
	int frontChan = 0;
	int lfe = a52_flags & A52_LFE ? 1 : 0;
	
	switch (a52_flags & A52_CHANNEL_MASK) {
		case A52_STEREO:
		case A52_DOLBY:
		case A52_2F1R:
		case A52_2F2R:
			//Handle right channel with no center
			chanMap[lfe+1] = fullChannelMap[3];
			frontChan = 1;

		case A52_CHANNEL:
		case A52_CHANNEL1:
		case A52_CHANNEL2:
		case A52_MONO:
			//Handle left channel
			chanMap[lfe] = fullChannelMap[1];
			frontChan++;
			break;
			
		case A52_3F:
		case A52_3F1R:
		case A52_3F2R:
			//Three front channels
			chanMap[lfe] = fullChannelMap[1];
			chanMap[lfe+1] = fullChannelMap[2];
			chanMap[lfe+2] = fullChannelMap[3];
			frontChan = 3;
			break;
			
		default:
			break;
	}
	if(lfe)
		chanMap[0] = fullChannelMap[0];
	chanMap[frontChan+lfe] = fullChannelMap[4];
	chanMap[frontChan+lfe+1] = fullChannelMap[5];
}

template <class outPtr, class inPtr>
UInt32 ACShepA52Decoder::InterleaveSamples(void *output_data_untyped, UInt32 output_data_offset, sample_t *output_samples, int a52_flags) {
	inPtr  *cast_samples;
	outPtr *output_data = (outPtr *)output_data_untyped;
	
	cast_samples = (inPtr *)output_samples;
	
	int chans = ChannelCount(a52_flags);
	int outputChans = mOutputFormat.mChannelsPerFrame;
	
	// each element is the liba52 channel number of the CA channel number of the index
	int chanMap[6];
	getChannelMap(a52_flags, chanMap);
	memset(output_data + output_data_offset, 0, outputChans * 256 * sizeof(outPtr));
	
	for (int i = 0; i < 256; i++) {
		for (int j = 0; j < chans; j++) {
			output_data[outputChans*i + output_data_offset + chanMap[j]] = cast_samples[i + 256*j];
		}
	}
	
	return outputChans * 256; // Number of 'UInt16' we processed
}


UInt32 ACShepA52Decoder::ProduceOutputPackets(void* outOutputData,
											   UInt32& ioOutputDataByteSize,
											   UInt32& ioNumberPackets, 
											   AudioStreamPacketDescription* outPacketDescription) {
	
	// Basically, we will copy the framework of the IMA decoder, and swap the changes
	// Most of this code is fairly straight from the command-line one I wrote
	
	// ioNumberPackets means how many input packets to process
	
	UInt32 theAnswer;								// Return value for function
	
	UInt32 bytes_to_read = 0;						// How many bytes the decoder will consume per A52 frame
    int a52_flags = 0;								// A report of the A52 stream channels
    int a52_samplerate = 0;							// What the rate of the A/52 stream is
    int a52_bitrate = 0;							// Bitrate of the A/52 Stream
	
	Byte *input_data = NULL;						// A convenience pointer to the input buffer to process
	sample_t *output_samples = NULL;				// A place to put the decoded samples (remember the non-interleaving)
	
	UInt32 available_frames = 0;					// How many frames of input are in the buffer
	UInt32 processed_frames = 0;					// How many frames have been processed
	UInt32 frames_to_process = 0;					// How many frames we should process
	
	sample_t level = 1;								// Range of +/- 1
	int bias = 0;									// Centered at 0
	
	int output_offset = 0;							// Offset within the output to write to
	UInt32 output_sample_size = 0;					// How big one sample of one channel is in the output
	
	UInt32 processedBytes = 0;						// How many bytes of data we have created
	
	
	if(!mIsInitialized) {
		CODEC_THROW(kAudioCodecStateError);
	}
	
	
	
	/*
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Being asked for %ld packets\n", ioNumberPackets);
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Current buffer usage: %ld bytes\n", GetUsedInputBufferByteSize());
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Given %ld bytes of output buffer\n", ioOutputDataByteSize);
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: outPacketDescription is %sNULL\n", (outPacketDescription == NULL ? "" : "not "));
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: processing an output of %u channels\n", (unsigned int)mOutputFormat.mChannelsPerFrame);
	*/
	
	
	// OK, we are told to process ioNumberPackets... can we?
	// We need to 'prime the pump' and start the decoder on the data, so we can get the info needed
	
	
	// A quick sanity check, to make sure the buffer has data
	if (GetUsedInputBufferByteSize() <= 0) {
		// Tell the system we need more friggin data
		
		ioOutputDataByteSize = 0;
		ioNumberPackets = 0;
		return kAudioCodecProduceOutputPacketNeedsMoreInputData;
	}
	
	
	// Get some of the data, in order to calculate sizes
	UInt32 readLength = 7;
	input_data = GetBytes(readLength);
	if (readLength != 7 || SyncA52Stream(bytes_to_read, input_data, a52_flags, a52_samplerate, a52_bitrate, true) ==
		kAudioCodecProduceOutputPacketNeedsMoreInputData) {
		
		ioOutputDataByteSize = 0;
		ioNumberPackets = 0;
		return kAudioCodecProduceOutputPacketNeedsMoreInputData;
	}
	//	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Synced Correctly!\n");
	
	
	// After all that, we now have info about the encoding of this stream
	// Available frames denotes number of input frames
	available_frames = (GetUsedInputBufferByteSize() / bytes_to_read);
	//	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Available input frames: %ld \n", available_frames);
	
	
	if (available_frames < ioNumberPackets) {
		// So, we can't make enough
		// We make what we can, then return a need for more input
		
		//		fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Cannot make as many packets as requested\n");
		frames_to_process = available_frames;
		theAnswer = kAudioCodecProduceOutputPacketNeedsMoreInputData;
	} else if (available_frames > ioNumberPackets) {
		// Aren't we just great!
		// Tell that we have more left
		
		//		fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Can make more packets than requested\n");
		frames_to_process = ioNumberPackets;
		theAnswer = kAudioCodecProduceOutputPacketSuccessHasMore;
	} else {
		// Aren't we just average!
		// Exactly right amount
		
		//		fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Can only make as many packets as requested\n");
		frames_to_process = ioNumberPackets;
		theAnswer = kAudioCodecProduceOutputPacketSuccess;
	}
	
	// Need to know what kind of output data to tally up	
	if (mOutputFormat.mFormatFlags == kIntPCMOutFormatFlag) {
		if (mOutputFormat.mBitsPerChannel == 16) {
			output_sample_size = sizeof(UInt16);
		} else {
			output_sample_size = sizeof(UInt32);
		}
	} else {
		output_sample_size = sizeof(float);
	}
	
	UInt32 theOutputByteSize = frames_to_process * 6 * 256 * mOutputFormat.mChannelsPerFrame * output_sample_size;	
	ThrowIf(ioOutputDataByteSize < theOutputByteSize, static_cast<ComponentResult>(kAudioCodecNotEnoughBufferSpaceError), "ACAppleIMA4Decoder::ProduceOutputPackets: not enough space in the output buffer");

	if (mOutputFormat.mFormatFlags == kIntPCMOutFormatFlag) {
		if (mOutputFormat.mBitsPerChannel == 16) {
			level = 1;
			bias = 384;
		} else {
			level = 1 << 30;
			bias = 0;
		}
	} else {
		level = 1;
		bias = 0;
	}
		
	while(processed_frames < frames_to_process) {
		// Redo the entry, for each loop
		
		input_data = GetBytes(readLength);
		if (readLength != 7 || SyncA52Stream(bytes_to_read, input_data, a52_flags, a52_samplerate, a52_bitrate, true) ==
			kAudioCodecProduceOutputPacketNeedsMoreInputData) {
			
			// Each PCM frame = (A52 frames) * (6 blocks/A52 frame) * (256 samples/block) * (# channels) * (sample bytes/channel * sample)
			processedBytes = processed_frames * 6 * 256 * mOutputFormat.mChannelsPerFrame * output_sample_size;	
			
			
			total_frames += processed_frames;
			total_bytes += processedBytes;
			
			ioNumberPackets = processed_frames;
			ioOutputDataByteSize = processedBytes;
			outPacketDescription = NULL;
			
			return kAudioCodecProduceOutputPacketNeedsMoreInputData;
		}
		
		// various data provided by a52 
		
		/*
		printf("ProduceOutputPackets: A52DEC: Bytes to read: %li\n", bytes_to_read);
		printf("ProduceOutputPackets: A52DEC: Flags: %x\n", a52_flags);
		printf("ProduceOutputPackets: A52DEC: Samplerate: %i\n", a52_samplerate);
		printf("ProduceOutputPackets: A52DEC: Bitrate: %i\n", a52_bitrate);
		*/

		// Now ready to do a bit of processing...
		
		if(passthrough)
		{
			static const uint8_t p_sync_le[6] = { 0x72, 0xF8, 0x1F, 0x4E, 0x01, 0x00 };
			static const uint8_t p_sync_be[6] = { 0xF8, 0x72, 0x4E, 0x1F, 0x00, 0x01 };
			
			uint8_t *myOutputData = (uint8_t *)(outOutputData);
			
			myOutputData += output_offset * output_sample_size;  //output_offset is in 16-bit ints
			
			int frameSize = mOutputFormat.mChannelsPerFrame * 2;
			memset(myOutputData, 0, mOutputFormat.mChannelsPerFrame * output_sample_size * 256 * 6);
			input_data = GetBytes(bytes_to_read);
			//Get the locations of the left and right channels in the output
			int leftOffset = fullChannelMap[1] * 2;
			int rightOffset = fullChannelMap[3] * 2;
			
			if (mOutputFormat.mFormatFlags & kLinearPCMFormatFlagIsBigEndian)
			{
				myOutputData[leftOffset] = p_sync_be[0];
				myOutputData[leftOffset+1] = p_sync_be[1];
				myOutputData[rightOffset] = p_sync_be[2];
				myOutputData[rightOffset+1] = p_sync_be[3];
				myOutputData[frameSize+leftOffset] = input_data[5] & 0x7;
				myOutputData[frameSize+leftOffset+1] = p_sync_be[5];
				myOutputData[frameSize+rightOffset] = (bytes_to_read >> 5) & 0xff;
				myOutputData[frameSize+rightOffset+1] = (bytes_to_read << 3) & 0xff;
				unsigned int i;
				int frameNumber;
				for(i=0, frameNumber = 2; i<bytes_to_read; i+=4, frameNumber++)
				{
					int offset = frameNumber * frameSize;
					myOutputData[offset+leftOffset] = input_data[i];
					myOutputData[offset+leftOffset+1] = input_data[i+1];
					myOutputData[offset+rightOffset] = input_data[i+2];
					myOutputData[offset+rightOffset+1] = input_data[i+3];
				}
			} 
			else
			{
				myOutputData[leftOffset] = p_sync_le[0];
				myOutputData[leftOffset+1] = p_sync_le[1];
				myOutputData[rightOffset] = p_sync_le[2];
				myOutputData[rightOffset+1] = p_sync_le[3];
				myOutputData[frameSize+leftOffset] = p_sync_le[4];
				myOutputData[frameSize+leftOffset+1] = input_data[5] & 0x7;
				myOutputData[frameSize+rightOffset] = (bytes_to_read << 3) & 0xff;
				myOutputData[frameSize+rightOffset+1] = (bytes_to_read >> 5) & 0xff;
				unsigned int i;
				int frameNumber;
				for(i=0, frameNumber = 2; i<bytes_to_read; i+=4, frameNumber++)
				{
					int offset = frameNumber * frameSize;
					myOutputData[offset+leftOffset] = input_data[i+1];
					myOutputData[offset+leftOffset+1] = input_data[i];
					myOutputData[offset+rightOffset] = input_data[i+3];
					myOutputData[offset+rightOffset+1] = input_data[i+2];
				}
			}
			output_offset += mOutputFormat.mChannelsPerFrame * 256 * 6;	//Our framed hack
		}
		else
		{
			if(mOutputFormat.mChannelsPerFrame <= 2)
			{
				if(mOutputFormat.mChannelsPerFrame == 1)
					// Just mono
					a52_flags = A52_MONO | A52_ADJUST_LEVEL;
				else
					// All we really need is stereophonic, baby
					a52_flags = TwoChannelMode;
			}
			else if(mOutputFormat.mChannelsPerFrame == ChannelCount(a52_flags))
				//Hope and pray that the channel layout is correct
				a52_flags |= A52_ADJUST_LEVEL;
			else
			{
				//Lion likes to use the largest number of channels even on stereo input; so let's not print this anymore
//				fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: channel count doesn't match; expect odd results\n");
				//Put in some guesses here
				switch(mOutputFormat.mChannelsPerFrame)
				{
					case 3:
						a52_flags = A52_2F1R | A52_ADJUST_LEVEL;
						break;
					case 4:
						a52_flags = A52_2F2R | A52_ADJUST_LEVEL;
						break;
					case 5:
						a52_flags = A52_3F2R | A52_ADJUST_LEVEL;
						break;
					case 6:
						a52_flags = A52_3F2R | A52_LFE | A52_ADJUST_LEVEL;
						break;
				}
			}

			readLength = bytes_to_read;
			input_data = GetBytes(readLength);
			a52_frame(decoder_state, input_data, &a52_flags, &level, bias);
			a52_dynrng(decoder_state, dynrng_call, &dynamicRangeCompression);
			
			// Cycle through the blocks, and actually do stuff 
			for (int j = 0; j < 6; j++) {
				a52_block(decoder_state);
				output_samples = a52_samples(decoder_state);
				
				// Need to know what kind of output data to process	
				if (mOutputFormat.mFormatFlags == kIntPCMOutFormatFlag) {
					if (mOutputFormat.mBitsPerChannel == 16) {
						/* Use a bias of 384 and a level of 1 to get the range, 383(+) to 385(-).
						   In IEEE floating point, the range is 0x0x43bf8001 to 0x43bf7fff.
						   If you cast these to ints, the range is -32767 to 32767*/
						output_offset += InterleaveSamples<SInt16, SInt32>(outOutputData, output_offset, output_samples, a52_flags);
					} else {
						output_offset += InterleaveSamples<SInt32, float>(outOutputData, output_offset, output_samples, a52_flags);
					}
				} else {
					output_offset += InterleaveSamples<float, float>(outOutputData, output_offset, output_samples, a52_flags);
				}
			
			}
		}
		
		// move on in our data stream
		ConsumeInputData(bytes_to_read);
		processed_frames++;
	}
	
	// Each PCM frame = (A52 frames) * (6 blocks/A52 frame) * (256 samples/block) * (# channels) * (sample bytes/channel * sample)
	processedBytes = processed_frames * 6 * 256 * mOutputFormat.mChannelsPerFrame * output_sample_size;			
				
	
	if (processedBytes > ioOutputDataByteSize) {
		fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Not enough output buffer allocated! Bad things happen now.\n");
	}
	
	total_frames += processed_frames;
	total_bytes += processedBytes;
	
	ioNumberPackets = processed_frames;
	ioOutputDataByteSize = processedBytes; 
	outPacketDescription = NULL;
	
	/*
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Processed %ld frames of input\n", processed_frames);
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Created %ld bytes of output\n", ioOutputDataByteSize);
	fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Bytes left in the buffer: %ld\n", GetUsedInputBufferByteSize());
	 */
	
	return theAnswer;
}


UInt32 ACShepA52Decoder::SyncA52Stream(UInt32 &bytes_to_read,
										unsigned char *input_data,
										int &a52_flags,
										int &a52_samplerate,
										int &a52_bitrate,
										bool shouldResync) {
	
	//fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Attempting sync\n");
	
	if (GetUsedInputBufferByteSize() < 7) {
		// Need 7 bytes to calc anything...
		return kAudioCodecProduceOutputPacketNeedsMoreInputData;
	}
	
	bytes_to_read = a52_syncinfo(input_data, &a52_flags, &a52_samplerate, &a52_bitrate);
	
	UInt32 retries = 0;
	
	// oops, somehow off a frame boundary... 
	// or maybe some other crazy stuff... 
	// as per suggestion, we will move up a byte in the buffer 
	// we will die if we dont have the data needed 
	while (bytes_to_read == 0) {
		
		
		if (shouldResync == false) {
			// Auto-dying
			return kAudioCodecProduceOutputPacketNeedsMoreInputData;
		}
		
		retries++;
		ConsumeInputData(1);
		
		if (GetUsedInputBufferByteSize() < 7) {
			fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Frame boundary problem\n");
			fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Ran out of buffer while resyncing\n");
			
			// Tell the caller we need more friggin data
			return kAudioCodecProduceOutputPacketNeedsMoreInputData;
			
		}
		
		UInt32 readLength = 7;
		input_data = GetBytes(readLength);
		bytes_to_read = a52_syncinfo(input_data, &a52_flags, &a52_samplerate, &a52_bitrate);
	}
	
	if (retries != 0) {
		fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Frame boundary problem\n");
		fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Took %d retries.\n", (int)retries);
	}
	
	return kAudioCodecProduceOutputPacketSuccess;
}

UInt32 ACShepA52Decoder::AppendPacket(const void* inInputData,
									  UInt32 inInputDataSize,
									  UInt32 bufferStartOffset,
									  UInt32 offset,
									  UInt32& packetSize)
{
	UInt32 bytes_to_read = 0;
	packetSize = 0;
	offset --;
	while(bytes_to_read == 0)
	{
		int packetFlags;
		int packetSampleRate;
		int packetBitrate;
		
		offset++;
		if(offset + 7 > inInputDataSize)
			break;
		bytes_to_read = a52_syncinfo(static_cast<const uint8_t*>(inInputData) + bufferStartOffset + offset, &packetFlags, &packetSampleRate, &packetBitrate);
	}
	if(bytes_to_read == 0)
		//Broke out of previous loop
		return offset;
	
	packetSize = bytes_to_read;
	if(bytes_to_read + offset > inInputDataSize)
		return offset;
	UInt32 bytes_can_copy = GetInputBufferByteSize() - GetUsedInputBufferByteSize();
	if(bytes_to_read > bytes_can_copy)
		return offset;
	
	ACSimpleCodec::AppendInputBuffer(inInputData, bufferStartOffset + offset, bytes_to_read);
	// fprintf(stderr, "ACShepA52Codec::AppendInputData: Copied in %ld:%ld new bytes\n", bytes_to_read, offset);
	
	return offset+bytes_to_read;
}

void ACShepA52Decoder::AppendInputData(const void* inInputData,
									   UInt32& ioInputDataByteSize,
									   UInt32& ioNumberPackets,
									   const AudioStreamPacketDescription* inPacketDescription) {
	
	//fprintf(stderr, "ACShepA52Decoder::AppendInputData: Appending data\n");
	
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: Before calling inner append, values are:\n");
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: inInputData points to: %x\n", inInputData);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: ioInputDataByteSize: %ld\n", ioInputDataByteSize);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: ioNumberPackets: %ld\n", ioNumberPackets);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: inPacketDescription points to: %x\n", inPacketDescription);
	
	
	// fprintf(stderr, "ACShepA52Codec::AppendInputData: Told to copy in %ld:%ld new bytes\n", ioNumberPackets, ioInputDataByteSize);
	if(ioInputDataByteSize == 0)
		//Nothing to do!
		return;
	
	UInt32 totalSize = GetInputBufferByteSize();
    UInt32 bytes_can_copy = totalSize - GetUsedInputBufferByteSize();

	if(inPacketDescription != NULL) {
		UInt32 packets = 0;
		
		while(packets < ioNumberPackets) {
			UInt32 packetSize = inPacketDescription[packets].mDataByteSize;
			if(packetSize > totalSize - GetUsedInputBufferByteSize())
				break;

			//Last frame was incommplete, complete it first.
			if(beginningOfIncompleteHeaderSize != 0)
			{
				if(packetSize + beginningOfIncompleteHeaderSize > 7)
				{
					//lets complete the header and party
					for(UInt32 i=0; i<beginningOfIncompleteHeaderSize; i++)
					{
						Byte newHeader[7];
						memcpy(newHeader, beginningOfIncompleteHeader + i, beginningOfIncompleteHeaderSize-i);
						memcpy(newHeader+beginningOfIncompleteHeaderSize-i, inInputData, 7-beginningOfIncompleteHeaderSize+i);
						
						int packetFlags;
						int packetSampleRate;
						int packetBitrate;
						
						int bytes_to_read = a52_syncinfo(newHeader, &packetFlags, &packetSampleRate, &packetBitrate);
						if(bytes_to_read != 0)
						{
							//First add the stuff stuck in the buffer
							beginningOfIncompleteHeaderSize -= i;
							ACSimpleCodec::AppendInputBuffer(static_cast<const void *>(beginningOfIncompleteHeader), (UInt32)i, beginningOfIncompleteHeaderSize);
							//Now, let the below handle the rest
							remainingBytesFromLastFrame = bytes_to_read - beginningOfIncompleteHeaderSize;
						}
					}
				}
				beginningOfIncompleteHeaderSize = 0;
			}
			
			UInt32 bytes_to_read = remainingBytesFromLastFrame;
			if(bytes_to_read > packetSize)
			{
				bytes_to_read = packetSize;
				remainingBytesFromLastFrame -= packetSize;
			}
			else
				remainingBytesFromLastFrame = 0;
			if(bytes_to_read != 0)
			{
				ACSimpleCodec::AppendInputBuffer(inInputData,inPacketDescription[packets].mStartOffset, bytes_to_read);
			}
			
			UInt32 offset = bytes_to_read;
			
			UInt32 frameSize = 0;
			while(offset < packetSize)
			{
				UInt32 offsetIn = offset;
				offset = AppendPacket(inInputData, packetSize, inPacketDescription[packets].mStartOffset, offset, frameSize);
				if(offsetIn == offset)
					break;
			}
			if(offset != packetSize)
			{
				if(packetSize - offset < 7)
				{
					//Incomplete header
					beginningOfIncompleteHeaderSize = packetSize - offset;
					memcpy(beginningOfIncompleteHeader, static_cast<const uint8_t*>(inInputData) + offset + inPacketDescription[packets].mStartOffset, beginningOfIncompleteHeaderSize);
				}
				else if(frameSize != 0)
				{
					UInt32 availableInFrame = packetSize - offset;
					ACSimpleCodec::AppendInputBuffer(inInputData, inPacketDescription[packets].mStartOffset + offset, availableInFrame);
					remainingBytesFromLastFrame = frameSize - availableInFrame;
				}
			}
			packets++;
		}
		ioNumberPackets = packets;
		ioInputDataByteSize = bytes_can_copy - (totalSize - GetUsedInputBufferByteSize());
	} else if (bytes_can_copy > 0) {
        UInt32 packet = 0;
		UInt32 offset = 0;
        while (packet < ioNumberPackets) {
			UInt32 packetSize = 0;
			offset = AppendPacket(inInputData, ioInputDataByteSize, 0, offset, packetSize);
			if(packetSize != 0)
				packet++;
			else
				break;
		}
		ioInputDataByteSize = offset;
		ioNumberPackets = packet;
	}
	if( ioInputDataByteSize == 0) {
        CODEC_THROW(kAudioCodecNotEnoughBufferSpaceError);
    }
	//fprintf(stderr, "ACShepA52Codec::AppendInputData: Copying in %ld:%ld new bytes\n", ioNumberPackets, ioInputDataByteSize);
	
	if (firstInput && ioInputDataByteSize != 0) {
		DetermineStreamParameters();
	}
	
	
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: After calling inner append, values are:\n");
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: inInputData points to: %x\n", inInputData);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: ioInputDataByteSize : %ld\n", ioInputDataByteSize);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: ioNumberPackets: %ld\n", ioNumberPackets);
	//	fprintf(stderr, "ACShepA52Decoder::AppendInputData: inPacketDescription points to: %x\n", inPacketDescription);
}

void ACShepA52Decoder::DetermineStreamParameters() {
	UInt32 bytes_to_read = 0;   // How many bytes the decoder will consume
	int a52_flags = 0;			// Info about the channels
	int a52_samplerate = 0;		// Sample rate of incoming audio
	int a52_bitrate = 0;		// Bitrate of incoming audio
	int total_channels = 0;		// Keep tally of total channels found
	Byte *input_data;			// Convenince pointer
	
	if (GetUsedInputBufferByteSize() < 7) {
		return;
	}
		
	UInt32 readLength = 7;
	input_data = GetBytes(readLength);
	if (SyncA52Stream(bytes_to_read, input_data, a52_flags, a52_samplerate, a52_bitrate, true) == kAudioCodecProduceOutputPacketNeedsMoreInputData) {
		
		fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Cannot set input stream info from appended data...\n");
		return;
	}
	
	firstInput = false;
	
	//fprintf(stderr, "ACShepA52Decoder::AppendInputData: Setting info gained from probing %ld bytes\n", bytes_to_read);
	mInputFormat.mBytesPerPacket = bytes_to_read;
	mInputFormat.mSampleRate = a52_samplerate;
	mInputFormat.mFramesPerPacket = 256*6;
	mInputFormat.mBytesPerFrame = mInputFormat.mBytesPerPacket / mInputFormat.mFramesPerPacket;
	
	total_channels = ChannelCount(a52_flags);
	
	//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found %d total channels\n", total_channels);
	
}

UInt32	ACShepA52Decoder::GetVersion() const {
	return 0x00017071;
}

extern "C"
ComponentResult	ACShepA52DecoderEntry(ComponentParameters* inParameters, ACShepA52Decoder* inThis) {	
	return	ACCodecDispatch(inParameters, inThis);
}

