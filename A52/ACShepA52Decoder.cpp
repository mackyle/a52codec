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


ACShepA52Decoder::ACShepA52Decoder(UInt32 inInputBufferByteSize) : ACShepA52Codec(inInputBufferByteSize) {
	
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
	
	for (int channels = 1; channels <= 6; channels++) {
		//	This decoder only takes an A/52 or AC-3 stream as it's input
		CAStreamBasicDescription theInputFormat1(48000, kAudioFormatAC3, 0, 256*6, 0, channels, 0, 0);
		AddInputFormat(theInputFormat1);
		CAStreamBasicDescription theInputFormat2(48000, kAudioFormatAVIAC3, 0, 256*6, 0, channels, 0, 0);
		AddInputFormat(theInputFormat2);
		
		// Output 16-Bit Ints
		CAStreamBasicDescription theOutputFormat1(48000, kAudioFormatLinearPCM, 0, 1, 0, channels, 16, kIntPCMOutFormatFlag);
		AddOutputFormat(theOutputFormat1);
		
		// And 32-Bit
		CAStreamBasicDescription theOutputFormat2(48000, kAudioFormatLinearPCM, 0, 1, 0, channels, 32, kIntPCMOutFormatFlag);
		AddOutputFormat(theOutputFormat2);
		
		// And floats
		CAStreamBasicDescription theOutputFormat3(48000, kAudioFormatLinearPCM, 0, 1, 0, channels, 32, kFloatPCMOutFormatFlag);
		AddOutputFormat(theOutputFormat3);
	}
	
	
	total_bytes = 0;
	total_frames = 0;
	decoder_state = NULL;
	firstInput = true;
	
	remainingBytesFromLastFrame = 0;
	beginningOfIncompleteHeaderSize = 0;
	
	CFPreferencesAppSynchronize(CFSTR("com.cod3r.a52codec"));
	CFTypeRef dynRange = CFPreferencesCopyAppValue(CFSTR("dynamicRange"), CFSTR("com.cod3r.a52codec"));
	if(dynRange != NULL)
	{
		CFTypeID type = CFGetTypeID(dynRange);
		if(type == CFStringGetTypeID())
			dynamicRangeCompression = CFStringGetDoubleValue((CFStringRef)dynRange);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)dynRange, kCFNumberDoubleType, &dynamicRangeCompression);
		else
			dynamicRangeCompression = 1;
	}
	else
		dynamicRangeCompression = 1;  //no compression
	
	CFTypeRef stereo = CFPreferencesCopyAppValue(CFSTR("useStereoOverDolby"), CFSTR("com.cod3r.a52codec"));
	if(stereo != NULL)
	{
		CFTypeID type = CFGetTypeID(dynRange);
		if(type == CFStringGetTypeID())
			dynamicRangeCompression = CFStringGetIntValue((CFStringRef)dynRange);
		else if(type == CFNumberGetTypeID())
			CFNumberGetValue((CFNumberRef)dynRange, kCFNumberIntType, &dynamicRangeCompression);
		else
			useStereoOverDolby = 0;		
	}
	else
		useStereoOverDolby = 0;
	
	//fprintf(stderr, "ACShepA52Decoder::Constructor: Number of input formats supported: %lu\n", GetNumberSupportedInputFormats());
	//fprintf(stderr, "ACShepA52Decoder::Constructor: Number of output formats supported: %lu\n", GetNumberSupportedOutputFormats());
}

ACShepA52Decoder::~ACShepA52Decoder() {
	// fprintf(stderr, "ACShepA52Decoder::Destructor: Total Frames: %ld\n", total_frames);
	// fprintf(stderr, "ACShepA52Decoder::Destructor: Total Output Bytes: %ld\n", total_bytes);
}

void ACShepA52Decoder::Initialize(const AudioStreamBasicDescription* inInputFormat,
									 const AudioStreamBasicDescription* inOutputFormat,
									 const void* inMagicCookie, UInt32 inMagicCookieByteSize) {
	
	ACShepA52Codec::Initialize(inInputFormat, inOutputFormat, inMagicCookie, inMagicCookieByteSize);
	
	//fprintf(stderr, "ACShepA52Decoder::Initialize: Initializing, magic cookie size is %lu\n", inMagicCookieByteSize);
	
	// Library setup
	decoder_state = a52_init(0); // No optimizations here
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
	decoder_state = a52_init(0);
	
	firstInput = true;
	remainingBytesFromLastFrame = beginningOfIncompleteHeaderSize = 0;
	
	//	let our base class clean up it's internal state
	ACShepA52Codec::Reset();
}

void ACShepA52Decoder::GetPropertyInfo(AudioCodecPropertyID inPropertyID, UInt32& outPropertyDataSize, bool& outWritable) {
	//fprintf(stderr, "ACShepA52Decoder::GetPropertyInfo: Asking for property %.*s\n", (int) sizeof(inPropertyID), (char*)&inPropertyID);
	
	switch(inPropertyID)
	{
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

			
		default:
			
			ACShepA52Codec::GetProperty(inPropertyID, ioPropertyDataSize, outPropertyData);
	}
}



void ACShepA52Decoder::SetProperty(AudioCodecPropertyID inPropertyID, UInt32 inPropertyDataSize, const void* inPropertyData)
{
	//fprintf(stderr, "ACShepA52Decoder::SetProperty: Asking for property %.*s\n", (int) sizeof(inPropertyID), (char*)&inPropertyID);

	switch(inPropertyID) {
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
	if(inInputFormat.mFormatID != kAudioFormatAC3 && inInputFormat.mFormatID != kAudioFormatAVIAC3) {
		DebugMessage("ACShepA52Decoder::SetFormats: only support A/52 and AC-3 for input");
		CODEC_THROW(kAudioCodecUnsupportedFormatError);
	}
	
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

static sample_t dynrng_call (sample_t c, void *data)
{
	double *level = (double *)data;
	return pow((double)c, *level);
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
			level = 2 << 30;
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
		
		switch(mOutputFormat.mChannelsPerFrame) {
			case 1:
				// Just mono
				a52_flags = A52_MONO | A52_ADJUST_LEVEL;
				break;
				
			case 2:
				// All we really need is stereophonic, baby
				if(useStereoOverDolby)
					a52_flags = A52_STEREO | A52_ADJUST_LEVEL;
				else
					a52_flags = A52_DOLBY | A52_ADJUST_LEVEL;
				break;
				
			case 5:
				// Try to get 5.0 channels
				a52_flags = A52_3F2R | A52_ADJUST_LEVEL;	
				break;
				
			case 6:
				// Try to get 5.1 channels
				a52_flags = A52_3F2R | A52_LFE | A52_ADJUST_LEVEL;
				break;
				
			default:
				fprintf(stderr, "ACShepA52Decoder::ProduceOutputPackets: Unknown output channel amount\n");
				break;
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
					output_offset += Process16BitSignedInts(outOutputData, output_offset, output_samples, a52_flags);
				} else {
					output_offset += Process32BitSignedInts(outOutputData, output_offset, output_samples, a52_flags);
				}
			} else {
				output_offset += ProcessFloats(outOutputData, output_offset, output_samples, a52_flags);
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


// Output order of liba52: LFE, left, center, right, left surround, right surround.
// Channels missing are skipped
// Supposed input to CoreAudio is: left, right, center, LFE, left surround, right surround

UInt32 ACShepA52Decoder::Process16BitSignedInts(void *output_data_untyped, UInt32 output_data_offset, sample_t *output_samples, int a52_flags) {
	SInt32  *cast_samples;
	SInt16 *output_data = (SInt16 *)output_data_untyped;
	
	cast_samples = (SInt32 *)output_samples;

	// Weird things...
	// As far as I can see, we take a float, cast it to an UInt32, then copy it to a UInt16
	// The trick being: level = 1 and bias = 384
	
	// It is 256 samples
	
	// ME.f is using 16-bit
	
	switch(a52_flags & A52_CHANNEL_MASK) {		
		
		// 5 Channel
		case A52_3F2R:
			
			// With LFE
			if (a52_flags & A52_LFE) {
				//fprintf(stderr, "ACShepA52Decoder::Process16BitSignedInts: Running 5.1\n");
				
				for (int k = 0; k < 256; k++) {
					output_data[6*k + output_data_offset + 3] =  cast_samples[k + 256*0]; // LFE chan 
					output_data[6*k + output_data_offset + 0] =  cast_samples[k + 256*1]; // left chan 
					output_data[6*k + output_data_offset + 2] =  cast_samples[k + 256*2]; // center chan 
					output_data[6*k + output_data_offset + 1] =  cast_samples[k + 256*3]; // right chan 
					output_data[6*k + output_data_offset + 4] =  cast_samples[k + 256*4]; // left surround chan 
					output_data[6*k + output_data_offset + 5] =  cast_samples[k + 256*5]; // right surround chan 
				}
				
				return 6 * 256; // Number of 'UInt16' we processed
				
			} else {
				//fprintf(stderr, "ACShepA52Decoder::Process16BitSignedInts: Running 5.0\n");
				for (int k = 0; k < 256; k++) {
					output_data[5*k + output_data_offset + 0] = cast_samples[k + 256*0]; // left chan 
					output_data[5*k + output_data_offset + 2] = cast_samples[k + 256*1]; // center chan 
					output_data[5*k + output_data_offset + 1] = cast_samples[k + 256*2]; // right chan 
					output_data[5*k + output_data_offset + 3] = cast_samples[k + 256*3]; // left surround chan 
					output_data[5*k + output_data_offset + 4] = cast_samples[k + 256*4]; // right surround chan 
				}
				
				return 5 * 256; // Number of 'UInt16' we processed
			}
			break;
			
		// Stereo
		case A52_STEREO:
		case A52_DOLBY: 
			//fprintf(stderr, "ACShepA52Decoder::Process16BitSignedInts: Running stereo\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[2*k + output_data_offset]     =  cast_samples[k];       // left chan 
				output_data[2*k + output_data_offset + 1] =  cast_samples[k + 256]; // right chan 
			}
				
			return 2 * 256; // Number of 'UInt16' we processed
			
			break;
			
		// Mono
		case A52_MONO:
			//fprintf(stderr, "ACShepA52Decoder::Process16BitSignedInts: Running mono\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[k + output_data_offset]     =  cast_samples[k];       // mono chan 
			}
			
			return 256; // Number of 'UInt16' we processed
			
			break;
			
		default:
			fprintf(stderr, "ACShepA52Decoder::Process16BitSignedInts: Failed to match output channels\n");
	}
	
	return 0;
}

// Output order of liba52: LFE, left, center, right, left surround, right surround.
// Channels missing are skipped
// Supposed input to CoreAudio is: left, right, center, LFE, left surround, right surround

UInt32 ACShepA52Decoder::Process32BitSignedInts(void *output_data_untyped, UInt32 output_data_offset, sample_t *output_samples, int a52_flags) {
	SInt32 *output_data = (SInt32 *)output_data_untyped;
	
	//fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Flags are: 0x%08X\n", a52_flags);
	
	// PlayAudioFileLite uses SInt32
	
	switch(a52_flags & A52_CHANNEL_MASK) {		
		
		// 5 Channel
		case A52_3F2R:
			
			// With LFE
			if (a52_flags & A52_LFE) {
				//fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Running 5.1\n");
				
				for (int k = 0; k < 256; k++) {
					output_data[6*k + output_data_offset + 3] = (SInt32)output_samples[k + 256*0]; // LFE chan 
					output_data[6*k + output_data_offset + 0] = (SInt32)output_samples[k + 256*1]; // left chan 
					output_data[6*k + output_data_offset + 2] = (SInt32)output_samples[k + 256*2]; // center chan 
					output_data[6*k + output_data_offset + 1] = (SInt32)output_samples[k + 256*3]; // right chan 
					output_data[6*k + output_data_offset + 4] = (SInt32)output_samples[k + 256*4]; // left surround chan 
					output_data[6*k + output_data_offset + 5] = (SInt32)output_samples[k + 256*5]; // right surround chan 
				}
				
				return 6 * 256; // Number of 'UInt32' we processed
			} else {
				//fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Running 5.0\n");
				for (int k = 0; k < 256; k++) {
					output_data[5*k + output_data_offset + 0] = (SInt32)output_samples[k + 256*0]; // left chan 
					output_data[5*k + output_data_offset + 2] = (SInt32)output_samples[k + 256*1]; // center chan 
					output_data[5*k + output_data_offset + 1] = (SInt32)output_samples[k + 256*2]; // right chan 
					output_data[5*k + output_data_offset + 3] = (SInt32)output_samples[k + 256*3]; // left surround chan 
					output_data[5*k + output_data_offset + 4] = (SInt32)output_samples[k + 256*4]; // right surround chan 
				}
				
				return 5 * 256; // Number of 'UInt32' we processed
			}
			break;
			
		// Stereo
		case A52_STEREO:
		case A52_DOLBY: 
			//fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Running stereo\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[2*k + output_data_offset]     =  (SInt32)output_samples[k];       // left chan 
				output_data[2*k + output_data_offset + 1] =  (SInt32)output_samples[k + 256]; // right chan 
			}
				
			return 2 * 256; // Number of 'UInt32' we processed
			break;
			
		// Mono
		case A52_MONO:
			//fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Running mono\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[k + output_data_offset]     =  (SInt32)output_samples[k];       // moon chan 
			}
			
			return 256; // Number of 'UInt32' we processed
			break;

		default:
			
			fprintf(stderr, "ACShepA52Decoder::Process32BitSignedInts: Failed to match output channels\n");
	}
	
	return 0;
}


UInt32 ACShepA52Decoder::ProcessFloats(void *output_data_untyped, UInt32 output_data_offset, sample_t *output_samples, int a52_flags) {
	float *output_data = (float *)output_data_untyped;
	
	
	switch(a52_flags & A52_CHANNEL_MASK) {		
		
		// 5 Channel
		case A52_3F2R:
			
			// With LFE
			if (a52_flags & A52_LFE) {
				//fprintf(stderr, "ACShepA52Decoder::ProcessFloats: Running 5.1\n");
				
				for (int k = 0; k < 256; k++) {
					output_data[6*k + output_data_offset + 3] = output_samples[k + 256*0]; // LFE chan 
					output_data[6*k + output_data_offset + 0] = output_samples[k + 256*1]; // left chan 
					output_data[6*k + output_data_offset + 2] = output_samples[k + 256*2]; // center chan 
					output_data[6*k + output_data_offset + 1] = output_samples[k + 256*3]; // right chan 
					output_data[6*k + output_data_offset + 4] = output_samples[k + 256*4]; // left surround chan 
					output_data[6*k + output_data_offset + 5] = output_samples[k + 256*5]; // right surround chan 
				}
				
				return 6 * 256; // Number of 'float' we processed
			} else {
				//fprintf(stderr, "ACShepA52Decoder::ProcessFloats: Running 5.0\n");
				for (int k = 0; k < 256; k++) {
					output_data[5*k + output_data_offset + 0] = output_samples[k + 256*0]; // left chan 
					output_data[5*k + output_data_offset + 2] = output_samples[k + 256*1]; // center chan 
					output_data[5*k + output_data_offset + 1] = output_samples[k + 256*2]; // right chan 
					output_data[5*k + output_data_offset + 3] = output_samples[k + 256*3]; // left surround chan 
					output_data[5*k + output_data_offset + 4] = output_samples[k + 256*4]; // right surround chan 
				}
				
				return 5 * 256; // Number of 'UInt32' we processed
			}
			
			break;
			
		// Stereo
		case A52_STEREO:
		case A52_DOLBY:
			//fprintf(stderr, "ACShepA52Decoder::ProcessFloats: Running stereo\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[2*k + output_data_offset]     =  output_samples[k];       // left chan 
				output_data[2*k + output_data_offset + 1] =  output_samples[k + 256]; // right chan 
			}
				
			return 2 * 256; // Number of 'float' we processed
			break;
			
		// Mono
		case A52_MONO:
			//fprintf(stderr, "ACShepA52Decoder::ProcessFloats: Running mono\n");
			
			for (int k = 0; k < 256; k++) {
				output_data[k + output_data_offset]     =  output_samples[k];       // mono chan 
			}
			
			return 256; // Number of 'float' we processed
			break;
			
		default:
			fprintf(stderr, "ACShepA52Decoder::ProcessFloats: Failed to match output channels\n");
	}
	return 0;
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
		fprintf(stderr, "ACShepA52Decoder::SyncA52Stream: Took %ld retries.\n", retries);
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
					ACSimpleCodec::AppendInputBuffer(inInputData, offset, availableInFrame);
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
	
	switch (a52_flags & A52_CHANNEL_MASK) {
		
		case (A52_CHANNEL): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found two independant monaural audio\n");
			total_channels += 2;
			break;
		}
			
		case (A52_CHANNEL1): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found first of two independant monaural channel audio\n");
			total_channels += 1;
			break;
		}			
			
		case (A52_CHANNEL2): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found second of two independant monaural channel audio\n");
			total_channels += 1;
			break;
		}
			
		case (A52_MONO): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found monaural audio\n");
			total_channels += 1;
			break;
		}
			
		case (A52_STEREO): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found stereo audio\n");
			total_channels += 2;
			break;
		}
			
		case (A52_DOLBY): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found Dolby Surround sound audio\n");
			total_channels += 2;
			break;
		}
			
		case (A52_3F): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front channel audio\n");
			total_channels += 3;
			break;
		}
			
		case (A52_2F1R): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 2 front & 1 back channel audio\n");
			total_channels += 3;
			break;
		}
			
		case (A52_3F1R): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front & 1 back channel audio\n");
			total_channels += 4;
			break;
		}
			
		case (A52_2F2R): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 2 front & 2 back channel audio\n");
			total_channels += 4;
			break;
		}
			
		case (A52_3F2R): {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found 3 front & 2 back channel audio\n");
			total_channels += 5;
			break;
		}
			
		default: {
			//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: No audio stream information found, probably not good!\n");
			break;
		}
			
			
	}
	
	if (a52_flags & A52_LFE) {
		//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found LFE channel in audio\n");
		total_channels += 1;
	}
	
	//fprintf(stderr, "ACShepA52Decoder::DetermineStreamParameters: Found %d total channels\n", total_channels);
	
}



UInt32	ACShepA52Decoder::GetVersion() const {
	return 0x00015000;
}

extern "C"
ComponentResult	ACShepA52DecoderEntry(ComponentParameters* inParameters, ACShepA52Decoder* inThis) {	
	return	ACCodecDispatch(inParameters, inThis);
}

